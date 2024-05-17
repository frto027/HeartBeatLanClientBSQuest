#include "HMUI/ViewController.hpp"
#include "ModConfig.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RenderMode.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils-methods.hpp"
#include "beatsaber-hook/shared/utils/typedefs-string.hpp"
#include "bsml/shared/BSML-Lite/Creation/Settings.hpp"
#include "bsml/shared/BSML-Lite/Creation/Text.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"
#include "bsml/shared/BSML/Components/Settings/DropdownListSetting.hpp"
#include "custom-types/shared/delegate.hpp"
#include "i18n.hpp"
#include "main.hpp"
#include "HeartBeatSetthings.hpp"
#include "HeartBeat.hpp"
#include "HeartBeatDataSource.hpp"

#include "HMUI/HierarchyManager.hpp"
#include "HMUI/ViewController.hpp"

#include "bsml/shared/BSML.hpp"
#include "sys/socket.h"
#include <cstddef>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string>
#include <string_view>
#include <vector>


namespace SetthingUI{
    HMUI::ViewController* setthings_controller;
    HMUI::ViewController* devices_controller;
    //============ pair / stop pair btn ==========
    UnityEngine::UI::Button * private_public_btn;
    bool private_ui = true;

    void UpdateSetthingsContent(){
            BSML::Lite::SetButtonText(private_public_btn, private_ui ? LANG->hiding_mac_addr: LANG->showing_mac_addr);
    }
    void PrivateNotPrivateBtnClick(){
        private_ui = !private_ui;
        UpdateSetthingsContent();
    }

    HeartBeat::HeartBeatObj * previewObj;

    void EnsurePreviewObject(){
        if(MainMenuPreviewObject == nullptr){
            auto obj = UnityEngine::GameObject::New_ctor();
            UnityEngine::Object::DontDestroyOnLoad(obj);
            auto canvas = obj->AddComponent<UnityEngine::Canvas*>();
            canvas->set_renderMode(UnityEngine::RenderMode::WorldSpace);
            auto crect = canvas->GetComponent<UnityEngine::RectTransform*>();
            crect->position = {-0.5, 0.5, 3};
            crect->sizeDelta = {1, 1};


            auto text = BSML::Lite::CreateText(canvas->get_transform(), "???");
            auto rect = text->get_rectTransform();
            rect->SetParent(canvas->transform, false);
            previewObj = text->get_gameObject()->AddComponent<HeartBeat::HeartBeatObj*>();
            rect->anchoredPosition = {0.5, 0.7};
            rect->sizeDelta = {1, 0.02};

            text->fontSize = 0.1;
            text->set_alignment(TMPro::TextAlignmentOptions::Center);

            MainMenuPreviewObject = obj;
        }
        if(MainMenuPreviewObject->get_active() == false)
            MainMenuPreviewObject->set_active(true);
    }

    void DidSetthingsActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        if(ModEnabled)
            EnsurePreviewObject();
        
        if(firstActivation) {

            setthings_controller = self;
            // Create a container that has a scroll bar
            auto *container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());


            BSML::Lite::CreateText(container->get_transform(),LANG->mod_version, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{100, 4});

            
            BSML::Lite::CreateToggle(container->get_transform(), LANG->enabled, getModConfig().Enabled.GetValue(), [](bool v){
                getModConfig().Enabled.SetValue(v);
            });

            if(ModEnabled == false)
                return;

            self->add_didDeactivateEvent(custom_types::MakeDelegate<HMUI::ViewController::DidDeactivateDelegate*>(std::function([](bool removedFromHierarchy, bool screenSystemDisabling){
                MainMenuPreviewObject->set_active(false);
            })));
            std::vector<std::string_view> languages = {
                "auto",
                "english",
                "chinese"};
            BSML::Lite::CreateDropdown(container->get_transform(),
                "Language(need restart)",getModConfig().ModLang.GetValue(),languages,[](StringW v){
                    getModConfig().ModLang.SetValue(v);
                } );

            // A data source toggle
            static std::vector<std::string_view> data_sources;
            data_sources = {
                LANG->data_source_random,
                LANG->data_source_lan,
                LANG->data_source_bluetooth,
                LANG->data_source_osc,
            };
            auto current_data_type = getModConfig().DataSourceType.GetValue();
            if(current_data_type >= data_sources.size()){
                current_data_type = 2;
            }
            if(current_data_type < 0)
                current_data_type = 2;
            BSML::Lite::CreateDropdown(container->get_transform(),
                LANG->data_source, 
                data_sources[current_data_type], data_sources, [](::StringW value){
                    for(int i=0;i<data_sources.size();i++){
                        if(data_sources[i] == value){
                            getLogger().debug("{} selected", i);
                            getModConfig().DataSourceType.SetValue(i);
                            break;
                        }
                    }
                    getLogger().debug("{} selected.", value);
                });

            private_public_btn =  BSML::Lite::CreateUIButton(container->get_transform(), LANG->waiting, UnityEngine::Vector2{}, UnityEngine::Vector2{200, 4}, PrivateNotPrivateBtnClick);

            static BSML::IncrementSetting *FlashDur;

            static UnityEngine::Color TextColor = getModConfig().HeartTextColor.GetValue();
            BSML::Lite::CreateColorPicker(container->get_transform(), LANG->text_color, getModConfig().HeartTextColor.GetValue(),
                [](UnityEngine::Color color){
                    getModConfig().HeartTextColor.SetValue(TextColor);
                    previewObj->text->color = TextColor;
                },
                [](){
                    TextColor = getModConfig().HeartTextColor.GetValue();
                    previewObj->text->color = TextColor;
                },
                [](UnityEngine::Color color){
                    TextColor = color;
                    previewObj->text->color = TextColor;
            });
            static UnityEngine::Color FlashTextColor = getModConfig().HeartDataComeFlashColor.GetValue();
            BSML::Lite::CreateColorPicker(container->get_transform(), LANG->flash_text_color, getModConfig().HeartDataComeFlashColor.GetValue(),
                [](UnityEngine::Color color){
                    getModConfig().HeartDataComeFlashColor.SetValue(FlashTextColor);
                    FlashTextColor = getModConfig().HeartTextColor.GetValue();
                },
                [](){
                    FlashTextColor = getModConfig().HeartTextColor.GetValue();
                },
                [](UnityEngine::Color color){
                    FlashTextColor = color;
            });

            FlashDur = BSML::Lite::CreateIncrementSetting(container->get_transform(), LANG->flash_duration_when_text_come, 1, 0.2, getModConfig().HeartDataComeFlashDuration.GetValue(), [](float v){
                if(v < 0){
                    v = 0;
                    FlashDur->__set_currentValue(0);
                    FlashDur->UpdateState();
                }
                getModConfig().HeartDataComeFlashDuration.SetValue(v);
            });
            BSML::Lite::CreateUIButton(container->get_transform(), LANG->flash_test, UnityEngine::Vector2{}, UnityEngine::Vector2{150, 8}, [](){
                previewObj->FlashColor();
            });
            BSML::Lite::CreateUIButton(container->get_transform(), LANG->reset_to_default, UnityEngine::Vector2{}, UnityEngine::Vector2{150, 8}, [](){
                getModConfig().HeartTextColor.SetValue(getModConfig().HeartTextColor.GetDefaultValue());
                previewObj->text->color = getModConfig().HeartTextColor.GetDefaultValue();

                getModConfig().HeartDataComeFlashColor.SetValue(getModConfig().HeartDataComeFlashColor.GetDefaultValue());
                
                getModConfig().HeartDataComeFlashDuration.SetValue(getModConfig().HeartDataComeFlashDuration.GetDefaultValue());
                FlashDur->currentValue = getModConfig().HeartDataComeFlashDuration.GetDefaultValue();
            });

            static char osc_port[4096];
            if(HeartBeat::dataSourceType == HeartBeat::DS_OSC){
                sprintf(osc_port, LANG->heart_osc_port, getModConfig().OSCPort.GetValue());
                BSML::Lite::CreateText(container->get_transform(),osc_port, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{100, 4});
            }

            UpdateSetthingsContent();
        }
    }

    namespace LanDataSource{
        HMUI::ViewController* servers_controller;
        HMUI::ViewController* devices_controller;
        BSML::CustomListTableData *server_list;
        std::vector<std::pair<unsigned int, unsigned short>> server_list_vec;
        void UpdateServerList(){
            auto* instance = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>();

            auto & servers = instance->paired_servers;
            std::lock_guard<std::mutex> lock(instance->mutex);

            bool any_data_changed = false;

            while(server_list->data.size() < servers.size()){
                server_list->data->Add(BSML::CustomCellInfo::construct(""));
                any_data_changed = true;
            }
            while(server_list->data.size() > servers.size()){
                server_list->data->RemoveAt(server_list->data.size()-1);
                any_data_changed = true;
            }
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
            getLogger().info("toggle server {}", idx);
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
                    getLogger().info("unknown IP addr {}", addr->sa_family);
                }
            }

            server_list->tableView->ClearSelection();
            UpdateServerList();
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

                while(ble_list->data.size() > ble_mac.size()){
                    ble_list->data->RemoveAt(ble_list->data.size() - 1);
                    any_data_changed = true;
                }
                while(ble_list->data.size() < ble_mac.size()){
                    ble_list->data->Add(BSML::CustomCellInfo::construct(""));
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
        void UpdateSelectedBLEValue(int idx){
            HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>()->SetSelectedBleMac(ble_mac[idx]);
            UpdateSelectedBLEScrollList();
        }

        void DidServersActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
            EnsurePreviewObject();
            if(firstActivation) {
                self->add_didDeactivateEvent(custom_types::MakeDelegate<HMUI::ViewController::DidDeactivateDelegate*>(std::function([](bool removedFromHierarchy, bool screenSystemDisabling){
                    MainMenuPreviewObject->set_active(false);
                })));
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
            EnsurePreviewObject();
            if(firstActivation) {
                self->add_didDeactivateEvent(custom_types::MakeDelegate<HMUI::ViewController::DidDeactivateDelegate*>(std::function([](bool removedFromHierarchy, bool screenSystemDisabling){
                    MainMenuPreviewObject->set_active(false);
                })));
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

    }

    namespace BleDataSource{
        BSML::CustomListTableData *ble_list;
        std::vector<std::string> ble_mac;

        void UpdateSelectedBLEScrollList(){
            auto * i = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatBleDataSource>();
            bool any_data_changed = false;
            int the_selected = -1;
            {
                std::set<std::string> already_in(ble_mac.begin(), ble_mac.end());
                auto& devs = i->avaliable_devices;

                for(auto it = devs.begin(), end = devs.end(); it != end; ++it){
                    if(already_in.count(it->first))
                        continue;
                    ble_mac.push_back(it->first);
                    already_in.insert(it->first);
                }

                while(ble_list->data.size() > ble_mac.size()){
                    ble_list->data->RemoveAt(ble_list->data.size() - 1);
                    any_data_changed = true;
                }
                while(ble_list->data.size() < ble_mac.size()){
                    ble_list->data->Add(BSML::CustomCellInfo::construct(""));
                    any_data_changed = true;
                }

                for(int j=0;j<ble_mac.size();j++){
                    bool selected = (ble_mac[j] == i->GetSelectedBleMac());
                    std::string name;

                    if(ble_mac[j] == ""){
                        name = selected ? ">>None" : "  None";
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
        void UpdateSelectedBLEValue(int idx){
            HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatBleDataSource>()->SetSelectedBleMac(ble_mac[idx]);
            UpdateSelectedBLEScrollList();
        }
        void DidDevicesActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
            EnsurePreviewObject();
            if(firstActivation) {
                self->add_didDeactivateEvent(custom_types::MakeDelegate<HMUI::ViewController::DidDeactivateDelegate*>(std::function([](bool removedFromHierarchy, bool screenSystemDisabling){
                    MainMenuPreviewObject->set_active(false);
                })));
                devices_controller = self;
                // Create a container that has a scroll bar
                auto *container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());

                BSML::Lite::CreateUIButton(container->get_transform(), LANG->scan_devices, UnityEngine::Vector2{}, UnityEngine::Vector2{200, 4}, [](){
                    auto instance=HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatBleDataSource>(); 
                    instance->ScanDevice();
                    UpdateSelectedBLEScrollList();
                });


                ble_list = BSML::Lite::CreateScrollableList(container->get_transform(), {70,60}, UpdateSelectedBLEValue);
                ble_list->set_listStyle(BSML::CustomListTableData::ListStyle::Simple);
                ble_list->tableView->set_selectionType(HMUI::TableViewSelectionType::Single);
                ble_mac.push_back("");
                UpdateSelectedBLEScrollList();
            }
        }
    }

    namespace OscDataSource{
        HMUI::ViewController *addrController;
        BSML::CustomListTableData *osc_list;
        std::vector<std::string> osc_addr;

        void UpdateOscScrollList(){
            auto * i = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatOSCDataSource>();
            bool any_data_changed = false;
            int the_selected = -1;
            {
                std::set<std::string> already_in(osc_addr.begin(), osc_addr.end());
                auto& devs = i->received_addresses;

                for(auto it = devs.begin(), end = devs.end(); it != end; ++it){
                    if(already_in.count(*it))
                        continue;
                    osc_addr.push_back(*it);
                    already_in.insert(*it);
                }

                while(osc_list->data.size() > osc_addr.size()){
                    osc_list->data->RemoveAt(osc_list->data.size() - 1);
                    any_data_changed = true;
                }
                while(osc_list->data.size() < osc_addr.size()){
                    osc_list->data->Add(BSML::CustomCellInfo::construct(""));
                    any_data_changed = true;
                }

                for(int j=0;j<osc_addr.size();j++){
                    bool selected = (osc_addr[j] == i->GetSelectedAddress());
                    std::string name;

                    name = std::string(selected ? ">>" : "  ") + osc_addr[j];
                    if(osc_list->data[j]->text != name){
                        osc_list->data[j]->text = name;
                        any_data_changed = true;
                    }
                    if(selected){
                        the_selected = j;
                    }
                }
            }
            if(any_data_changed)
                osc_list->tableView->ReloadData();
            if(the_selected >= 0){
                osc_list->tableView->SelectCellWithIdx(the_selected, false);
            }
        }
        void UpdateSelectedOscValue(int idx){
            HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatOSCDataSource>()->SetSelectedAddr(osc_addr[idx]);
            UpdateOscScrollList();
        }
        void DidDevicesActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
            EnsurePreviewObject();
            if(firstActivation) {
                addrController = self;
                self->add_didDeactivateEvent(custom_types::MakeDelegate<HMUI::ViewController::DidDeactivateDelegate*>(std::function([](bool removedFromHierarchy, bool screenSystemDisabling){
                    MainMenuPreviewObject->set_active(false);
                })));
                devices_controller = self;
                // Create a container that has a scroll bar
                auto *container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());

                osc_list = BSML::Lite::CreateScrollableList(container->get_transform(), {70,60}, UpdateSelectedOscValue);
                osc_list->set_listStyle(BSML::CustomListTableData::ListStyle::Simple);
                osc_list->tableView->set_selectionType(HMUI::TableViewSelectionType::Single);
                osc_addr.push_back("None");
                UpdateOscScrollList();
            }
        }
    }

    //Called from HeartBeat::Update
    void UpdateSetthingsUI(){
        if(HeartBeat::dataSourceType == HeartBeat::DS_LAN){
            // update lan devices
            if(LanDataSource::servers_controller && LanDataSource::servers_controller->get_isActivated())
                LanDataSource::UpdateServerList();
            if(devices_controller && devices_controller->get_isActivated())
                LanDataSource::UpdateSelectedBLEScrollList();
        }
        if(HeartBeat::dataSourceType == HeartBeat::DS_OSC){
            if(OscDataSource::addrController && OscDataSource::addrController->get_isActivated())
                OscDataSource::UpdateOscScrollList();
        }
    }

    void Setup(){
        BSML::Register::RegisterMainMenuViewControllerMethod("HeartBeatLan", LANG->heart_config, "<3", SetthingUI::DidSetthingsActivate);
        if(ModEnabled == false)
            return;
        if(HeartBeat::dataSourceType == HeartBeat::DS_BLE){
            BSML::Register::RegisterMainMenuViewControllerMethod(
                "HeartBeatLan", LANG->heart_devices,"<3", 
                SetthingUI::BleDataSource::DidDevicesActivate);
        }
        if(HeartBeat::dataSourceType == HeartBeat::DS_LAN){
            BSML::Register::RegisterMainMenuViewControllerMethod(
                "HeartBeatLan", LANG->heart_devices,"<3", 
                SetthingUI::LanDataSource::DidDevicesActivate);
            BSML::Register::RegisterMainMenuViewControllerMethod(
                "HeartBeatLan", LANG->heart_senders, "<3",
                SetthingUI::LanDataSource::DidServersActivate);
        }
        if(HeartBeat::dataSourceType == HeartBeat::DS_OSC){
            BSML::Register::RegisterMainMenuViewControllerMethod(
                "HeartBeatLan", LANG->heart_osc_senders, "<3",
                SetthingUI::OscDataSource::DidDevicesActivate);

        }
    }
}