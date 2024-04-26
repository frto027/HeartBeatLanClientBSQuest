#include "main.hpp"
#include "HeartBeat.hpp"
#include "HeartBeatDataSource.hpp"
#include "HeartBeatSetthings.hpp"

#include "bsml/shared/BSML.hpp"
#include "paper/shared/logger.hpp"

#include "HMUI/HierarchyManager.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/MonoBehaviour.hpp"

#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"

#include "UnityEngine/Quaternion.hpp"

#include "custom-types/shared/macros.hpp"

#include "ModConfig.hpp"

#include "i18n.hpp"

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





// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    I18N::Setup();

    il2cpp_functions::Init();

    BSML::Init();

    Paper::Logger::fmtLog<Paper::LogLevel::INF>("Installing ui...");
    SetthingUI::Setup();

    auto logger = Paper::ConstLoggerContext("HeartBeatLanClientQuestHooks");

    Paper::Logger::fmtLog<Paper::LogLevel::INF>("Installing hooks...");
    INSTALL_HOOK(logger, HeartBeatUIInit);
    INSTALL_HOOK(logger, HeartBeatSceneChange)
    
    Paper::Logger::fmtLog<Paper::LogLevel::INF>("Installed all hooks!");
}