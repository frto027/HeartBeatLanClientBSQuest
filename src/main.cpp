#include "main.hpp"
#include "HeartBeat.hpp"
#include "HeartBeatDataSource.hpp"

#include "codegen/include/UnityEngine/GameObject.hpp"
#include "codegen/include/UnityEngine/MonoBehaviour.hpp"
#include "codegen/include/HMUI/HierarchyManager.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/CustomTypes/Components/ExternalComponents.hpp"

#include "codegen/include/UnityEngine/SceneManagement/SceneManager.hpp"

#include "custom-types/shared/macros.hpp"

#include "ModConfig.hpp"

#include <vector>
#include <set>

#include "sys/socket.h"
#include <netinet/in.h>
#include <netinet/ip.h>

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;
	
    getModConfig().Init(modInfo);
    getLogger().info("Completed setup!");
}

HeartBeat::HeartBeatObj *heartbeatObj = nullptr;

MAKE_HOOK_MATCH(HeartBeatUIInit, &HMUI::HierarchyManager::Start, void,HMUI::HierarchyManager * self){
    HeartBeatUIInit(self);
    
    getLogger().info("The hook point start!");

    static bool init = false;
    if(init)
        return;
    init = true;
    auto obj = UnityEngine::GameObject::New_ctor("the_heartbeat");
    UnityEngine::GameObject::DontDestroyOnLoad(obj);
    heartbeatObj = obj->AddComponent<HeartBeat::HeartBeatObj*>();
    getLogger().info("Heart object created.");
   //heartbeatObj->Hide();
}

MAKE_HOOK_MATCH(HeartBeatSceneChange, &UnityEngine::SceneManagement::SceneManager::SetActiveScene, bool, UnityEngine::SceneManagement::Scene scene){
    std::string name = scene.get_name();
    getLogger().info("Load the scene -> %s", name.c_str());
    if(name == "GameCore"){
        if(heartbeatObj)
            heartbeatObj->SetStatus(HEARTBEAT_STATUS_GAMECORE);
    }else if(name == "MainMenu"){
        if(heartbeatObj)
            heartbeatObj->SetStatus(HEARTBEAT_STATUS_MAINMENU);
    }else{
        if(heartbeatObj)
            heartbeatObj->SetStatus(HEARTBEAT_STATUS_HIDE);
    }
    return HeartBeatSceneChange(scene);
}

//--------------- the ui ----------------------

namespace ModUI{
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
                QuestUI::BeatSaberUI::SetButtonText(pair_stoppair_btn, "Pairing.. click to stop.");
            }else{
                QuestUI::BeatSaberUI::SetButtonText(pair_stoppair_btn, "Not pairing.. click to start.");
            }
            QuestUI::BeatSaberUI::SetButtonText(private_public_btn, private_ui ? "hiding mac address, click to show.": "showing mac address, click to hide");
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

    QuestUI::CustomListTableData *ble_list;
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
                ble_list->data.resize(ble_mac.size());
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
                        name = std::string(selected ? ">>" : "  ") + ("Unknown(") + ble_mac[j] + ")";
                    }
                }
                if(ble_list->data[j].text != name){
                    ble_list->data[j].text = name;
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
    QuestUI::CustomListTableData *server_list;
    std::vector<std::pair<unsigned int, unsigned short>> server_list_vec;
    void UpdateServerList(){
        auto* instance = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>();

        auto & servers = instance->paired_servers;
        std::lock_guard<std::mutex> lock(instance->mutex);

        bool any_data_changed = false;

        if(server_list->data.size() != servers.size())
            any_data_changed = true;

        server_list->data.resize(servers.size());
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
                sprintf(buff, "%s %d.%d.%d.%d:%d", server.ignored ? "skip ":"     ", (int)((a>>24) &0xFF), (int)((a>>16)&0xFF), (int)((a>>8)&0xFF), (int)(a&0xFF), p);
            }else{
                sprintf(buff, "%s unknown addr.", server.ignored ? "skip ": "     ");
            }
            if(server_list->data[i].text != buff){
                server_list->data[i].text = buff;
                any_data_changed = true;
            }
        }
        if(any_data_changed){
            server_list->tableView->ReloadData();
        }
    }

    void SwitchServerIgnore(int idx){
        getLogger().info("toggle server %d", idx);
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
                getLogger().info("unknown IP addr %d", addr->sa_family);
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
            auto *container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());
            
            pair_stoppair_btn =  QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Waiting...", PairUnpairBtnClick);
            private_public_btn =  QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Waiting...", PrivateNotPrivateBtnClick);
            UpdateSetthingsContent();
        }
    }


    void DidServersActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        // Create our UI elements only when shown for the first time.
        if(firstActivation) {
            servers_controller = self;
            // Create a container that has a scroll bar
            auto *container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());

            server_list = QuestUI::BeatSaberUI::CreateScrollableList(container->get_transform(), {70, 60}, SwitchServerIgnore);
            server_list->set_listStyle(QuestUI::CustomListTableData::ListStyle::Simple);
            server_list->tableView->set_selectionType(HMUI::TableViewSelectionType::Single);
            UpdateServerList();
        }
    }
    void DidDevicesActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        // Create our UI elements only when shown for the first time.
        if(firstActivation) {
            devices_controller = self;
            // Create a container that has a scroll bar
            auto *container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());

            ble_list = QuestUI::BeatSaberUI::CreateScrollableList(container->get_transform(), {70,60}, UpdateSelectedBLEValue);
            ble_list->set_listStyle(QuestUI::CustomListTableData::ListStyle::Simple);
            ble_list->tableView->set_selectionType(HMUI::TableViewSelectionType::Single);
            ble_mac.push_back("");
            UpdateSelectedBLEScrollList();
        }
    }


}



// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    QuestUI::Init();
    
    getLogger().info("Installing quest ui...");
    //QuestUI::Register::RegisterMainMenuModSettingsViewController(modInfo, ModUI::DidActivate);
    QuestUI::Register::RegisterMainMenuModSettingsViewController(modInfo, "HEART CONFIG", ModUI::DidSetthingsActivate);
    QuestUI::Register::RegisterMainMenuModSettingsViewController(modInfo, "HEART DEVICES", ModUI::DidDevicesActivate);
    QuestUI::Register::RegisterMainMenuModSettingsViewController(modInfo, "HEART Servers", ModUI::DidServersActivate);
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), HeartBeatUIInit);
    INSTALL_HOOK(getLogger(), HeartBeatSceneChange)
    
    getLogger().info("Installed all hooks!");
}