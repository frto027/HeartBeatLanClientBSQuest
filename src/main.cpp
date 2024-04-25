#include "main.hpp"
#include "HeartBeat.hpp"
#include "HeartBeatDataSource.hpp"

#include "paper/shared/logger.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "HMUI/HierarchyManager.hpp"

#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"

#include "UnityEngine/Quaternion.hpp"

#include "HMUI/ViewController.hpp"

#include "custom-types/shared/macros.hpp"

#include "ModConfig.hpp"

#include <vector>
#include <set>

#include "sys/socket.h"
#include <netinet/in.h>
#include <netinet/ip.h>

static modloader::ModInfo modInfo = {MOD_ID, VERSION, 0}; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Called at the early stages of game loading
extern "C" void setup(CModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo.assign(info);
	
    getModConfig().Init(modInfo);
    Paper::Logger::fmtLog<Paper::LogLevel::INF>("Completed setup!");
}

HeartBeat::HeartBeatObj *heartbeatObj = nullptr;

MAKE_HOOK_MATCH(HeartBeatUIInit, &HMUI::HierarchyManager::Start, void,HMUI::HierarchyManager * self){
    HeartBeatUIInit(self);
    
    Paper::Logger::fmtLog<Paper::LogLevel::INF>("The hook point start!");

    static bool init = false;
    if(init)
        return;
    init = true;
    auto obj = UnityEngine::GameObject::New_ctor("the_heartbeat");
    UnityEngine::GameObject::DontDestroyOnLoad(obj);
    heartbeatObj = obj->AddComponent<HeartBeat::HeartBeatObj*>();
    Paper::Logger::fmtLog<Paper::LogLevel::INF>("Heart object created.");
   //heartbeatObj->Hide();
}

MAKE_HOOK_MATCH(HeartBeatSceneChange, &UnityEngine::SceneManagement::SceneManager::SetActiveScene, bool, UnityEngine::SceneManagement::Scene scene){
    std::string name = scene.get_name();
    Paper::Logger::fmtLog<Paper::LogLevel::INF>("Load the scene -> {}", name.c_str());
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

    void PairUnpairBtnClick(){
        auto * instance = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>();
        if(ui_is_pairing){
            instance->StopPair();
        }else{
            instance->StartPair();
        }
        // UpdateSetthingsContent();
    }
    void PrivateNotPrivateBtnClick(){
        private_ui = !private_ui;
        // UpdateSetthingsContent();
    }

    // QuestUI::CustomListTableData *ble_list;
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

            // if(ble_list->data.size() != ble_mac.size()){
            //     ble_list->data.resize(ble_mac.size());
            //     any_data_changed = true;
            // }
            
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
                // if(ble_list->data[j].text != name){
                //     ble_list->data[j].text = name;
                //     any_data_changed = true;
                // }
                if(selected){
                    the_selected = j;
                }
            }
        }
        // if(any_data_changed)
        //     ble_list->tableView->ReloadData();
        // if(the_selected >= 0){
        //     ble_list->tableView->SelectCellWithIdx(the_selected, false);
        // }
    }

    // server lists
    // QuestUI::CustomListTableData *server_list;
    std::vector<std::pair<unsigned int, unsigned short>> server_list_vec;
    void UpdateServerList(){
        auto* instance = HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatLanDataSource>();

        auto & servers = instance->paired_servers;
        std::lock_guard<std::mutex> lock(instance->mutex);

        bool any_data_changed = false;

        // if(server_list->data.size() != servers.size())
        //     any_data_changed = true;

        // server_list->data.resize(servers.size());
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
            // if(server_list->data[i].text != buff){
            //     server_list->data[i].text = buff;
            //     any_data_changed = true;
            // }
        }
        // if(any_data_changed){
        //     server_list->tableView->ReloadData();
        // }
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

        // server_list->tableView->ClearSelection();
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

}



// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    auto logger = Paper::ConstLoggerContext("HeartBeatLanClientQuestHooks");

    Paper::Logger::fmtLog<Paper::LogLevel::INF>("Installing hooks...");
    INSTALL_HOOK(logger, HeartBeatUIInit);
    INSTALL_HOOK(logger, HeartBeatSceneChange)
    
    Paper::Logger::fmtLog<Paper::LogLevel::INF>("Installed all hooks!");
}