#include "ModConfig.hpp"
#include "beatsaber-hook/shared/utils/typedefs-string.hpp"
#include "bsml/shared/BSML-Lite/Creation/Settings.hpp"
#include "bsml/shared/BSML/Components/Settings/DropdownListSetting.hpp"
#include "i18n.hpp"
#include "main.hpp"
#include "HeartBeatSetthings.hpp"
#include "HeartBeat.hpp"
#include "HeartBeatDataSource.hpp"

#include "HMUI/HierarchyManager.hpp"
#include "HMUI/ViewController.hpp"

#include "bsml/shared/BSML.hpp"
#include "sys/socket.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string>
#include <string_view>
#include <vector>


namespace SetthingUI{
    HMUI::ViewController* setthings_controller;
    HMUI::ViewController* servers_controller;
    HMUI::ViewController* devices_controller;
    //============ pair / stop pair btn ==========
    UnityEngine::UI::Button * pair_stoppair_btn;
    UnityEngine::UI::Button * private_public_btn;
    bool ui_is_pairing = false;
    bool private_ui = true;

    void UpdateSetthingsContent(){
            if((ui_is_pairing = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>()->IsPairing())){
                BSML::Lite::SetButtonText(pair_stoppair_btn, LANG->pairing);
            }else{
                BSML::Lite::SetButtonText(pair_stoppair_btn, LANG->not_pairing);
            }
            BSML::Lite::SetButtonText(private_public_btn, private_ui ? LANG->hiding_mac_addr: LANG->showing_mac_addr);
    }
    void PairUnpairBtnClick(){
        auto * instance = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>();
        if(ui_is_pairing){
            instance->StopPair();
        }else{
            instance->StartPair();
        }
        UpdateSetthingsContent();
    }
    void PrivateNotPrivateBtnClick(){
        private_ui = !private_ui;
        UpdateSetthingsContent();
    }

    BSML::CustomListTableData *ble_list;
    std::vector<std::string> ble_mac;

    void UpdateSelectedBLEScrollList(){
        auto * i = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>();
        bool any_data_changed = false;
        int the_selected = -1;
        {
            std::lock_guard<std::mutex> lock(i->mutex);
            std::set<std::string> already_in(ble_mac.begin(), ble_mac.end());

            auto& devs = i->avaliable_devices;

            for(auto it = devs.begin(), end = devs.end(); it != end; ++it){
                if(already_in.count(it->first))
                    continue;
                ble_mac.push_back(it->first);
                already_in.insert(it->first);
            }

            if(ble_list->data.size() != ble_mac.size()){
                ble_list->data->EnsureCapacity(ble_mac.size());
                any_data_changed = true;
            }
            
            for(int j=0;j<ble_mac.size();j++){
                bool selected = (ble_mac[j] == i->GetSelectedBleMac());
                std::string name;

                if(ble_mac[j] == ""){
                    name = selected ? ">>Any" : "  Any";
                }else{
                    auto it = devs.find(ble_mac[j]);
                    if(it != devs.end()){
                        name = std::string(selected ? ">>" : "  ") + (it->second.name) + "(" + 
                             (private_ui ? "XX-XX-XX-XX-XX-XX" : ble_mac[j])
                             + ")";
                    }else{
                        name = std::string(selected ? ">>" : "  ") + (LANG->unknown_left_quote) + ble_mac[j] + ")";
                    }
                }
                if(ble_list->data[j]->text != name){
                    ble_list->data[j]->text = name;
                    any_data_changed = true;
                }
                if(selected){
                    the_selected = j;
                }
            }
        }
        if(any_data_changed)
            ble_list->tableView->ReloadData();
        if(the_selected >= 0){
            ble_list->tableView->SelectCellWithIdx(the_selected, false);
        }
    }

    // server lists
    BSML::CustomListTableData *server_list;
    std::vector<std::pair<unsigned int, unsigned short>> server_list_vec;
    void UpdateServerList(){
        auto* instance = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>();

        auto & servers = instance->paired_servers;
        std::lock_guard<std::mutex> lock(instance->mutex);

        bool any_data_changed = false;

        if(server_list->data.size() != servers.size())
            any_data_changed = true;

        server_list->data->EnsureCapacity((int)(servers.size()));
        server_list_vec.resize(servers.size());

        
        for(int i=0;i<servers.size();i++){
            auto & server = servers.at(i);
            auto * addr = &server.addr;
            char buff[512];
            if(addr->sa_family == AF_INET){
                auto * iaddr = (sockaddr_in*)addr;
                unsigned int a = ntohl(iaddr->sin_addr.s_addr);
                unsigned short p = ntohs(iaddr->sin_port);
                server_list_vec[i] = std::pair(a,p);
                // sprintf(buff, server.ignored? "a ign server" : "a server");;
                sprintf(buff, "%s %d.%d.%d.%d:%d", server.ignored ? LANG->server_skip:"     ", (int)((a>>24) &0xFF), (int)((a>>16)&0xFF), (int)((a>>8)&0xFF), (int)(a&0xFF), p);
            }else{
                sprintf(buff, LANG->unknown_addr_server_fmt, server.ignored ? LANG->server_skip: "     ");
            }
            if(server_list->data[i]->text != buff){
                server_list->data[i]->text = buff;
                any_data_changed = true;
            }
        }
        if(any_data_changed){
            server_list->tableView->ReloadData();
        }
    }

    void SwitchServerIgnore(int idx){
        Paper::Logger::fmtLog<Paper::LogLevel::INF>("toggle server {}", idx);
        auto* instance = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>();

        auto & servers = instance->paired_servers;

        auto e = server_list_vec.at(idx);
        for(int i=0;i<servers.size();i++){
            auto & server = servers.at(i);
            auto * addr = &server.addr;
            char buff[256];
            if(addr->sa_family == AF_INET){
                auto * iaddr = (sockaddr_in*)addr;
                unsigned int a = ntohl(iaddr->sin_addr.s_addr);
                unsigned short p = ntohs(iaddr->sin_port);
                if(std::pair(a,p) == e){
                    server.ignored = !server.ignored;
                }
            }else{
                Paper::Logger::fmtLog<Paper::LogLevel::INF>("unknown IP addr {}", addr->sa_family);
            }
        }

        server_list->tableView->ClearSelection();
        UpdateServerList();
    }

    void UpdateSetthingsUI(){
        if(servers_controller && servers_controller->get_isActivated())
            UpdateServerList();
        if(devices_controller && devices_controller->get_isActivated())
            UpdateSelectedBLEScrollList();
    }

    void UpdateSelectedBLEValue(int idx){
        HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>()->SetSelectedBleMac(ble_mac[idx]);
        UpdateSelectedBLEScrollList();
    }

    void DidSetthingsActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        if(firstActivation) {
            setthings_controller = self;
            // Create a container that has a scroll bar
            auto *container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());
            
            BSML::Lite::CreateText(container->get_transform(),std::string(LANG->heart_rate_lan_protocol_ver) + HeartBeat::HeartBeatLanDataSource::GetProtocolVersion());
            BSML::Lite::CreateText(container->get_transform(),LANG->mod_version);

            pair_stoppair_btn =  BSML::Lite::CreateUIButton(container->get_transform(), LANG->waiting, PairUnpairBtnClick);
            private_public_btn =  BSML::Lite::CreateUIButton(container->get_transform(), LANG->waiting, PrivateNotPrivateBtnClick);
            
            #define SPLIT(x) do{\
                BSML::Lite::CreateText(container->get_transform(), "----- " x " -----")->set_alignment(TMPro::TextAlignmentOptions::Bottom);\
            }while(0)

            SPLIT("This Config Menu");

            BSML::DropdownListSetting * languageSelect;
            std::vector<std::string_view> languages = {
                "auto",
                "english",
                "chinese"};
            languageSelect = BSML::Lite::CreateDropdown(container->get_transform(),
                "Language",getModConfig().ModLang.GetValue(),languages,[](StringW v){
                    getModConfig().ModLang.SetValue(v);
                } );

            static BSML::IncrementSetting *MenuPosX, *MenuPosY, *MenuPosZ, *MenuRotY, *GameCoreX, *GameCoreY, *GameCoreZ, *GameCoreRotY;
            static BSML::IncrementSetting *FlashDur;

            BSML::Lite::CreateIncrementSetting(container->get_transform(), LANG->config_adjust_speed, 2, 0.01, 0.02, [](float v){
                MenuPosX->increments = v;
                MenuPosY->increments = v;
                MenuPosZ->increments = v;
                GameCoreX->increments = v;
                GameCoreY->increments = v;
                GameCoreZ->increments = v;
                MenuRotY->increments = v * 100;
                GameCoreRotY->increments = v * 100;
                FlashDur->increments = v * 10;
            })->minValue = 0.01;

            SPLIT("Text Color");

            BSML::Lite::CreateColorPicker(container->get_transform(), LANG->text_color, getModConfig().HeartTextColor.GetValue(),
                [](UnityEngine::Color color){
                    getModConfig().HeartTextColor.SetValue(heartbeatObj->text->get_color());

                },
                [](){
                    heartbeatObj->text->set_color(getModConfig().HeartTextColor.GetValue());
                },
                [](UnityEngine::Color color){
                    heartbeatObj->text->set_color(color);
            });

            BSML::Lite::CreateColorPicker(container->get_transform(), LANG->flash_text_color, getModConfig().HeartDataComeFlashColor.GetValue(),
                [](UnityEngine::Color color){
                    getModConfig().HeartDataComeFlashColor.SetValue(heartbeatObj->text->get_color());
                    heartbeatObj->text->set_color(getModConfig().HeartTextColor.GetValue());
                },
                [](){
                    heartbeatObj->text->set_color(getModConfig().HeartTextColor.GetValue());
                },
                [](UnityEngine::Color color){
                    heartbeatObj->text->set_color(color);
            });

            FlashDur = BSML::Lite::CreateIncrementSetting(container->get_transform(), LANG->flash_duration_when_text_come, 1, 0.2, getModConfig().HeartDataComeFlashDuration.GetValue(), [](float v){
                if(v < 0){
                    v = 0;
                    FlashDur->__set_currentValue(0);
                    FlashDur->UpdateState();
                }
                getModConfig().HeartDataComeFlashDuration.SetValue(v);
                heartbeatObj->FlashColor();
            });

            BSML::Lite::CreateUIButton(container->get_transform(), LANG->flash_test, [](){
                heartbeatObj->FlashColor();
            });

            SPLIT("Text");

            static BSML::IncrementSetting *LineSpace;
            LineSpace = BSML::Lite::CreateIncrementSetting(container->get_transform(), LANG->line_space, 0, 1, getModConfig().HeartLineSpaceDelta.GetValue(), [](float v){
                getModConfig().HeartLineSpaceDelta.SetValue(v);
                heartbeatObj->text->set_lineSpacing(v);
                heartbeatObj->SetStatus(HEARTBEAT_STATUS_GAMECORE);
            });

            BSML::Lite::CreateUIButton(container->get_transform(), LANG->reset_default_line_space, [](){
                auto v = getModConfig().HeartLineSpaceDelta.GetDefaultValue();
                getModConfig().HeartLineSpaceDelta.SetValue(v);
                heartbeatObj->text->set_lineSpacing(v);
                LineSpace->__set_currentValue(v);
                LineSpace->UpdateState();
                heartbeatObj->SetStatus(HEARTBEAT_STATUS_GAMECORE);
            });


            SPLIT("Game Play Position");


            //======================== Game Play Position ===============

            GameCoreRotY = BSML::Lite::CreateIncrementSetting(container->get_transform(),"Game Play Rot Y", 0, 2, getModConfig().HeartGameCoreRot.GetValue(), [](float v){
                getModConfig().HeartGameCoreRot.SetValue(v);
                heartbeatObj->SetStatus(HEARTBEAT_STATUS_GAMECORE);
            });

            GameCoreX = BSML::Lite::CreateIncrementSetting(container->get_transform(),"Game Play X", 2, 0.02, 
                getModConfig().HeartGameCorePos.GetValue().x, [](float v){
                    auto & p = getModConfig().HeartGameCorePos;
                    auto vec = p.GetValue();
                    vec.x = v;
                    p.SetValue(vec);
                    heartbeatObj->SetStatus(HEARTBEAT_STATUS_GAMECORE);
            });

            GameCoreY = BSML::Lite::CreateIncrementSetting(container->get_transform(),"Game Play Y", 2, 0.02, 
                getModConfig().HeartGameCorePos.GetValue().y, [](float v){
                    auto & p = getModConfig().HeartGameCorePos;
                    auto vec = p.GetValue();
                    vec.y = v;
                    p.SetValue(vec);
                    heartbeatObj->SetStatus(HEARTBEAT_STATUS_GAMECORE);
            });

            GameCoreZ = BSML::Lite::CreateIncrementSetting(container->get_transform(),"Game Play Z", 2, 0.02, 
                getModConfig().HeartGameCorePos.GetValue().z, [](float v){
                    auto & p = getModConfig().HeartGameCorePos;
                    auto vec = p.GetValue();
                    vec.z = v;
                    p.SetValue(vec);
                    heartbeatObj->SetStatus(HEARTBEAT_STATUS_GAMECORE);
            });


            BSML::Lite::CreateUIButton(container->get_transform(), "Game Play Position: Reset Default", [](){
                auto & conf = getModConfig();
                auto d = conf.HeartGameCorePos.GetDefaultValue();
                conf.HeartGameCorePos.SetValue(d);
                GameCoreX->__set_currentValue(d.x);
                GameCoreY->__set_currentValue(d.y);
                GameCoreZ->__set_currentValue(d.z);
                auto defaultRot = conf.HeartGameCoreRot.GetDefaultValue();
                GameCoreRotY->__set_currentValue(defaultRot);
                conf.HeartGameCoreRot.SetValue(defaultRot);
                GameCoreX->UpdateState();
                GameCoreY->UpdateState();
                GameCoreZ->UpdateState();
                GameCoreRotY->UpdateState();
                heartbeatObj->SetStatus(HEARTBEAT_STATUS_GAMECORE);
            });

            BSML::Lite::CreateUIButton(container->get_transform(), "Game Play Position: Go To", [](){
                heartbeatObj->SetStatus(HEARTBEAT_STATUS_GAMECORE);
            });

            //======================== Main Menu Position ===============
            SPLIT("Main Menu Position");

            MenuRotY = BSML::Lite::CreateIncrementSetting(container->get_transform(),"Main Menu Rot Y", 0, 2, getModConfig().HeartMainMenuRot.GetValue(), [](float v){
                getModConfig().HeartMainMenuRot.SetValue(v);
                heartbeatObj->SetStatus(HEARTBEAT_STATUS_MAINMENU);
            });

            MenuPosX = BSML::Lite::CreateIncrementSetting(container->get_transform(),"Main Menu X", 2, 0.02, 
                getModConfig().HeartMainMenuPos.GetValue().x, [](float v){
                    auto & p = getModConfig().HeartMainMenuPos;
                    auto vec = p.GetValue();
                    vec.x = v;
                    p.SetValue(vec);
                    heartbeatObj->SetStatus(HEARTBEAT_STATUS_MAINMENU);
            });

            MenuPosY = BSML::Lite::CreateIncrementSetting(container->get_transform(),"Main Menu Y", 2, 0.02, 
                getModConfig().HeartMainMenuPos.GetValue().y, [](float v){
                    auto & p = getModConfig().HeartMainMenuPos;
                    auto vec = p.GetValue();
                    vec.y = v;
                    p.SetValue(vec);
                    heartbeatObj->SetStatus(HEARTBEAT_STATUS_MAINMENU);
            });

            MenuPosZ = BSML::Lite::CreateIncrementSetting(container->get_transform(),"Main Menu Z", 2, 0.02, 
                getModConfig().HeartMainMenuPos.GetValue().z, [](float v){
                    auto & p = getModConfig().HeartMainMenuPos;
                    auto vec = p.GetValue();
                    vec.z = v;
                    p.SetValue(vec);
                    heartbeatObj->SetStatus(HEARTBEAT_STATUS_MAINMENU);
            });

            BSML::Lite::CreateUIButton(container->get_transform(), "Main Menu Position: Reset Default", [](){
                auto & conf = getModConfig();
                auto d = conf.HeartMainMenuPos.GetDefaultValue();
                conf.HeartMainMenuPos.SetValue(d);
                MenuPosX->__set_currentValue(d.x);
                MenuPosY->__set_currentValue(d.y);
                MenuPosZ->__set_currentValue(d.z);
                auto defaultRot = conf.HeartMainMenuRot.GetDefaultValue();
                MenuRotY->__set_currentValue(defaultRot);
                conf.HeartMainMenuRot.SetValue(defaultRot);
                MenuPosX->UpdateState();
                MenuPosY->UpdateState();
                MenuPosZ->UpdateState();
                MenuRotY->UpdateState();
                heartbeatObj->SetStatus(HEARTBEAT_STATUS_MAINMENU);
            });

            BSML::Lite::CreateUIButton(container->get_transform(), "Main Menu Position: GO TO", [](){
                heartbeatObj->SetStatus(HEARTBEAT_STATUS_MAINMENU);
            });

            UpdateSetthingsContent();
        }
    }


    void DidServersActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        // Create our UI elements only when shown for the first time.
        if(firstActivation) {
            servers_controller = self;
            // Create a container that has a scroll bar
            auto *container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());

            server_list = BSML::Lite::CreateScrollableList(container->get_transform(), {70, 60}, SwitchServerIgnore);
            server_list->set_listStyle(BSML::CustomListTableData::ListStyle::Simple);
            server_list->tableView->set_selectionType(HMUI::TableViewSelectionType::Single);
            UpdateServerList();
        }
    }
    void DidDevicesActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        // Create our UI elements only when shown for the first time.
        if(firstActivation) {
            devices_controller = self;
            // Create a container that has a scroll bar
            auto *container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());

            ble_list = BSML::Lite::CreateScrollableList(container->get_transform(), {70,60}, UpdateSelectedBLEValue);
            ble_list->set_listStyle(BSML::CustomListTableData::ListStyle::Simple);
            ble_list->tableView->set_selectionType(HMUI::TableViewSelectionType::Single);
            ble_mac.push_back("");
            UpdateSelectedBLEScrollList();
        }
    }

    void Setup(){
        BSML::Register::RegisterMainMenuViewControllerMethod("HeartBeatLan", "HEART Config", "<3", SetthingUI::DidSetthingsActivate);
        BSML::Register::RegisterMainMenuViewControllerMethod("HeartBeatLan", "HEART Devices","<3", SetthingUI::DidDevicesActivate);
        BSML::Register::RegisterMainMenuViewControllerMethod("HeartBeatLan", "HEART Senders", "<3",SetthingUI::DidServersActivate);
    }
}