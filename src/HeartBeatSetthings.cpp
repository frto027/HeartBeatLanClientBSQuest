#include "BeatLeaderRecorder.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/InputFieldView.hpp"
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
#include <mutex>
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

    void EnsurePreviewObject(){
        HeartBeat::HeartBeatObj * ret = nullptr;
        if(!MainMenuPreviewObject){
            HeartBeat::assetBundleMgr.Init();
            auto obj = UnityEngine::GameObject::New_ctor();
            UnityEngine::Object::DontDestroyOnLoad(obj);
            auto canvas = obj->AddComponent<UnityEngine::Canvas*>();
            canvas->set_renderMode(UnityEngine::RenderMode::WorldSpace);
            canvas->set_scaleFactor(0.001);
            auto crect = canvas->GetComponent<UnityEngine::RectTransform*>();
            crect->set_position({0, 1.5f, 3});
            crect->set_anchoredPosition({0.5f,0.5f});
            crect->set_localScale({0.01f,0.01f,0.01f});

            std::string SelectedUI = getModConfig().SelectedUI.GetValue();
            if(!HeartBeat::assetBundleMgr.loadedBundles.contains(SelectedUI))
                SelectedUI = "Default";
            if(!HeartBeat::assetBundleMgr.loadedBundles.contains(SelectedUI)){
                getLogger().error("Can't find ui asset bundle '{}' to load!", SelectedUI);
            }

            HeartBeat::AssetBundleInstinateInformation result;
            HeartBeat::assetBundleMgr.Instantiate(SelectedUI, canvas->get_transform(), result);
            (MainMenuPreviewObjectComp = result.gameObject->AddComponent<HeartBeat::HeartBeatObj*>())->loadedComponents = result;

            MainMenuPreviewObject = obj;
        }
        if(MainMenuPreviewObject->get_active() == false){
            MainMenuPreviewObject->set_active(true);
            HeartBeat::DataSource::getInstance()->SetDisplayWanted(true);
        }
    }

    void DidSetthingsActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        if(ModEnabled)
            EnsurePreviewObject();
        if(firstActivation) {
            HeartBeat::assetBundleMgr.Init();
            setthings_controller = self;
            // Create a container that has a scroll bar
            auto *container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());


            BSML::Lite::CreateText(container->get_transform(),LANG->mod_version, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 4});
            BSML::Lite::CreateText(container->get_transform(),LANG->for_game, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 8});
            #ifdef WITH_REPLAY
            BSML::Lite::CreateText(container->get_transform(),"build-feature: replay", 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 4});
            #endif
            BSML::Lite::CreateToggle(container->get_transform(), LANG->enabled, getModConfig().Enabled.GetValue(), [](bool v){
                getModConfig().Enabled.SetValue(v);
            });

            if(ModEnabled == false)
                return;

            self->add_didDeactivateEvent(custom_types::MakeDelegate<HMUI::ViewController::DidDeactivateDelegate*>(std::function([](bool removedFromHierarchy, bool screenSystemDisabling){
                if(MainMenuPreviewObject) MainMenuPreviewObject->set_active(false);
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
            //the value of data_sources MUST consist with the DS_*** enum
            data_sources = {
                LANG->data_source_random,
                LANG->data_source_lan,
                LANG->data_source_bluetooth,
                LANG->data_source_osc,
                LANG->data_source_hyperate,
                LANG->data_source_pulsoid
            };

            static std::vector<std::string_view> data_sources_in_ui;
            data_sources_in_ui = {
                LANG->data_source_bluetooth,
                LANG->data_source_hyperate,
                LANG->data_source_osc,
                LANG->data_source_pulsoid,
                LANG->data_source_random,
            };

            auto current_data_type = getModConfig().DataSourceType.GetValue();
            if(current_data_type >= data_sources.size() || current_data_type == HeartBeat::DS_LAN){
                current_data_type = HeartBeat::DS_BLE, getModConfig().DataSourceType.SetValue(current_data_type);
            }
            if(current_data_type < 0)
                current_data_type = HeartBeat::DS_BLE, getModConfig().DataSourceType.SetValue(current_data_type);
            BSML::Lite::CreateDropdown(container->get_transform(),
                LANG->data_source, 
                data_sources[current_data_type], data_sources_in_ui, [](::StringW value){
                    for(int i=0;i<data_sources.size();i++){
                        if(data_sources[i] == value){
                            getLogger().debug("{} selected", i);
                            getModConfig().DataSourceType.SetValue(i);
                            break;
                        }
                    }
                    getLogger().debug("{} selected.", value);
                });


            if(HeartBeat::Recorder::BeatLeaderDetected()){
                BSML::Lite::CreateToggle(container->get_transform(), LANG->enable_record, getModConfig().EnableRecord.GetValue(), [](bool v){
                    getModConfig().EnableRecord.SetValue(v);
                });
                BSML::Lite::CreateToggle(container->get_transform(), LANG->record_dev_name, getModConfig().RecordDevName.GetValue(), [](bool v){
                    getModConfig().RecordDevName.SetValue(v);
                });
            }else{
                BSML::Lite::CreateText(container->get_transform(),LANG->no_beatleader, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 8});
            }

            // the age is just used to provide a default value for maxheart
            static BSML::IncrementSetting * MaxHeartIncr;
            BSML::Lite::CreateIncrementSetting(container->get_transform(),
                LANG->age, 0, 1, getModConfig().Age.GetValue(), [](float v){
                    getModConfig().Age.SetValue(v);
                    MaxHeartIncr->set_Value(220 - v);
                    MaxHeartIncr->UpdateState();
                    getModConfig().MaxHeart.SetValue(220 - v);
                });
            MaxHeartIncr = BSML::Lite::CreateIncrementSetting(container->get_transform(),
                LANG->max_heart, 0, 1, getModConfig().MaxHeart.GetValue(), [](float v){
                    getModConfig().MaxHeart.SetValue(v);
                });

            std::vector<std::string_view> ui_s;
            for(auto& pair : HeartBeat::assetBundleMgr.loadedBundles){
                ui_s.push_back(pair.first);
            }

            static HMUI::CurvedTextMeshPro * feature_unsupport_hint_ui;

            BSML::Lite::CreateDropdown(container->get_transform(),
                LANG->select_ui, getModConfig().SelectedUI.GetValue(), ui_s, [](StringW v){
                    if(getModConfig().SelectedUI.GetValue() != v){
                        getModConfig().SelectedUI.SetValue(v);
                        if(MainMenuPreviewObject){
                            UnityEngine::Object::Destroy(MainMenuPreviewObject);
                            MainMenuPreviewObject = nullptr;
                            MainMenuPreviewObjectComp = nullptr;
                        }

                        bool supported = true;
                        {
                            HeartBeat::assetBundleMgr.Init();
                            auto it = HeartBeat::assetBundleMgr.loadedBundles.find(v);
                            if(it != HeartBeat::assetBundleMgr.loadedBundles.end()){
                                auto & features = it->second.unsupported_features;
                                if(features.size() > 0)
                                    supported = false;
                            }
                        }
                        feature_unsupport_hint_ui->set_text(supported ? "" : LANG->unsupported_feature_udpatre_mod);

                        EnsurePreviewObject();
                    }
                }
            );

            feature_unsupport_hint_ui = BSML::Lite::CreateText(container->get_transform(), "", 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 4});
            feature_unsupport_hint_ui->set_color(UnityEngine::Color::get_red());


            private_public_btn =  BSML::Lite::CreateUIButton(container->get_transform(), LANG->waiting, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 8}, PrivateNotPrivateBtnClick);

            static char osc_port[4096];
            if(HeartBeat::dataSourceType == HeartBeat::DS_OSC){
                sprintf(osc_port, LANG->heart_osc_port, getModConfig().OSCPort.GetValue());
                BSML::Lite::CreateText(container->get_transform(),osc_port, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 4});
            }

            BSML::Lite::CreateText(container->get_transform(),LANG->your_setthings_is_in_another_menu, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 10});

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
                    if(MainMenuPreviewObject) MainMenuPreviewObject->set_active(false);
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
                    if(MainMenuPreviewObject) MainMenuPreviewObject->set_active(false);
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
                    if(MainMenuPreviewObject) MainMenuPreviewObject->set_active(false);
                })));
                devices_controller = self;
                // Create a container that has a scroll bar
                auto *container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());

                BSML::Lite::CreateUIButton(container->get_transform(), LANG->scan_devices, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 8}, [](){
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
                    if(MainMenuPreviewObject) MainMenuPreviewObject->set_active(false);
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

    namespace HypeRateSource{

        std::string hyperate_id = "";

        void DidDevicesActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
            EnsurePreviewObject();
            if(firstActivation) {
                self->add_didDeactivateEvent(custom_types::MakeDelegate<HMUI::ViewController::DidDeactivateDelegate*>(std::function([](bool removedFromHierarchy, bool screenSystemDisabling){
                    if(MainMenuPreviewObject) MainMenuPreviewObject->set_active(false);
                    HeartBeat::DataSource::getInstance()->SetDisplayWanted(false);
                })));
                devices_controller = self;
                // Create a container that has a scroll bar
                auto *container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());

                hyperate_id = getModConfig().HypeRateId.GetValue();
                BSML::Lite::CreateText(container->get_transform(), LANG->hyperate_input_hint, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 4});
                // BSML::Lite::CreateText(container->get_transform(), LANG->hyperate_input_hint2, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 4});


                static HMUI::InputFieldView * hyperate_id_input;
                hyperate_id_input = BSML::Lite::CreateStringSetting(container->get_transform(), "HypeRate ID", hyperate_id, [](StringW v){
                    hyperate_id = std::string(v);
                });
                BSML::Lite::CreateUIButton(container->get_transform(), LANG->hyperate_reset, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 8}, [](){
                    {
                        hyperate_id = getModConfig().HypeRateId.GetValue();
                        hyperate_id_input->set_text(hyperate_id.c_str());
                    }
                    hyperate_id_input->set_text(hyperate_id);
                });
                BSML::Lite::CreateUIButton(container->get_transform(), LANG->hyperate_save_and_connect, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 8}, [](){
                    getModConfig().HypeRateId.SetValue(hyperate_id);
                    HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatHypeRateDataSource>()->ResetConnection();
                });
                if(MainMenuPreviewObjectComp)
                    MainMenuPreviewObjectComp->GetComponent<HeartBeat::HeartBeatObj *>()->serverMessageDisplayer = BSML::Lite::CreateText(container->get_transform(), LANG->no_message_from_server, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{100, 32});
            }
        }
    }

    namespace PulsoidSource{

        HMUI::ViewController *controller;

        HMUI::CurvedTextMeshPro* tokenText;
        bool tokenTextIsDirty = false;

        UnityEngine::UI::Button *PairInBrowserBtn, *BrowserCompleteBtn, *CancelBrowserPairBtn;

        HMUI::CurvedTextMeshPro* errMsgText;

        void DidDevicesActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
            EnsurePreviewObject();
            if(firstActivation) {
                controller = self;
                self->add_didDeactivateEvent(custom_types::MakeDelegate<HMUI::ViewController::DidDeactivateDelegate*>(std::function([](bool removedFromHierarchy, bool screenSystemDisabling){
                    if(MainMenuPreviewObject) MainMenuPreviewObject->set_active(false);
                    HeartBeat::DataSource::getInstance()->SetDisplayWanted(false);
                })));
                devices_controller = self;
                // Create a container that has a scroll bar
                auto *container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());

                BSML::Lite::CreateUIButton(container->get_transform(), LANG->pulsoid_reconnect, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 8}, [](){
                    HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatPulsoidDataSource>()->ResetConnection();
                });

                BSML::Lite::CreateText(container->get_transform(), LANG->pulsoid_input_hint, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 4});
                
                {
                    auto * pair_container = BSML::Lite::CreateHorizontalLayoutGroup(container->get_transform());
                    PairInBrowserBtn = BSML::Lite::CreateUIButton(pair_container->get_transform(), LANG->pulsoid_connect, UnityEngine::Vector2{}, UnityEngine::Vector2{20, 8}, [](){
                        HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatPulsoidDataSource>()->RequestSafePair();
                    });

                    BrowserCompleteBtn = BSML::Lite::CreateUIButton(pair_container->get_transform(), LANG->pulsoid_done, UnityEngine::Vector2{}, UnityEngine::Vector2{20, 8}, [](){
                        HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatPulsoidDataSource>()->SafePairDone();
                    });

                    CancelBrowserPairBtn = BSML::Lite::CreateUIButton(pair_container->get_transform(), LANG->pulsoid_cancel, UnityEngine::Vector2{}, UnityEngine::Vector2{20, 8}, [](){
                        HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatPulsoidDataSource>()->CancelSafePair();
                    });
                    BrowserCompleteBtn->set_interactable(false);
                    CancelBrowserPairBtn->set_interactable(false);
                }

                errMsgText = BSML::Lite::CreateText(container->get_transform(), "", 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 4});
                errMsgText->set_color(UnityEngine::Color::get_red());

                BSML::Lite::CreateText(container->get_transform(), LANG->pulsoid_token, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 4});
                tokenText = BSML::Lite::CreateText(container->get_transform(), "", 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 4});
                BSML::Lite::CreateUIButton(container->get_transform(), LANG->pulsoid_clear_token, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 8}, [](){
                    getModConfig().PulsoidToken.SetValue(getModConfig().PulsoidToken.GetDefaultValue());
                    HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatPulsoidDataSource>()->modconfig_is_dirty = true;
                    HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatPulsoidDataSource>()->ResetConnection();
                });

                {
                    std::string config_file_hint = LANG->pulsoid_edit_config_hint + modConfigFilePath;
                    char buff[1024];
                    int j = 0;
                    for(int i=0, ch_count = 0;i<config_file_hint.size();i++){
                        //please we are handling utf8 string
                        if((config_file_hint[i] & 0xC0) != 0x80){
                            ch_count++;
                            auto ch = config_file_hint[i];
                            if(ch_count > 40 && (ch == ' ' || ch == '/' || ch == '\\')){
                                buff[j++] = '\n';
                                ch_count = 0;
                            }
                            if((config_file_hint[i] & 0x80))
                                ch_count += 1;

                            if(config_file_hint[i] == '\n')
                                ch_count = 0;
                        }
                        buff[j++] = config_file_hint[i];
                    }
                    buff[j++] = '\0';
                    BSML::Lite::CreateText(container->get_transform(), buff, 4, UnityEngine::Vector2{}, UnityEngine::Vector2{50, 18});
                }

                HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatPulsoidDataSource>()->modconfig_is_dirty = true;
            }
        }

        void Update(){
            auto * ds = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatPulsoidDataSource>();
            if(ds->modconfig_is_dirty){
                ds->modconfig_is_dirty = false;
                
                {
                    std::string token = getModConfig().PulsoidToken.GetValue();
                    for(int i=8;i<token.size();i++){
                        if(token[i] != '-')
                            token[i] = '*';
                    }
                    tokenText->set_text(token);
                }
            }

            if(ds->err_message_dirty){
                std::lock_guard<std::mutex> g(ds->err_message_mutex);
                errMsgText->set_text(ds->err_message);
                ds->err_message_dirty = false;
            }
            if(ds->url_open_wanted){
                std::string url;
                {
                    std::lock_guard<std::mutex> g(ds->url_mutex);
                    url = ds->url;
                    ds->url_open_wanted = false;
                }
                //open the login url in the quest browser
                static auto UnityEngine_Application_OpenURL = il2cpp_utils::resolve_icall<void, StringW>("UnityEngine.Application::OpenURL");
                UnityEngine_Application_OpenURL(url);
            }
                if(PairInBrowserBtn){
                    PairInBrowserBtn->set_interactable(!ds->IsSafePairing());
                    BrowserCompleteBtn->set_interactable(ds->IsSafePairing());
                    CancelBrowserPairBtn->set_interactable(ds->IsSafePairing());
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
        if(HeartBeat::dataSourceType == HeartBeat::DS_Pulsoid){
            if(PulsoidSource::controller && PulsoidSource::controller->get_isActivated())
                PulsoidSource::Update();
            else{
                HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatPulsoidDataSource>()->CancelSafePair();
            }
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
            HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatBleDataSource>()->ScanDevice();
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
        if(HeartBeat::dataSourceType == HeartBeat::DS_HypeRate){
            BSML::Register::RegisterMainMenuViewControllerMethod(
                "HeartBeatLan", LANG->hyperate, "<3",
                SetthingUI::HypeRateSource::DidDevicesActivate);

        }
        if(HeartBeat::dataSourceType == HeartBeat::DS_Pulsoid){
            BSML::Register::RegisterMainMenuViewControllerMethod(
                "HeartBeatLan", LANG->pulsoid, "<3",
                SetthingUI::PulsoidSource::DidDevicesActivate);

        }
    }
}
