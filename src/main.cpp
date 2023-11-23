#include "main.hpp"
#include "HeartBeat.hpp"

#include "codegen/include/UnityEngine/GameObject.hpp"
#include "codegen/include/UnityEngine/MonoBehaviour.hpp"
#include "codegen/include/HMUI/HierarchyManager.hpp"
#include "questui/shared/QuestUI.hpp"
#include "codegen/include/UnityEngine/SceneManagement/SceneManager.hpp"

#include "custom-types/shared/macros.hpp"


static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
// other config tools such as config-utils don't use this config, so it can be removed if those are in use
Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

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
	
    getConfig().Load();
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


// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    QuestUI::Init();
    
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), HeartBeatUIInit);
    INSTALL_HOOK(getLogger(), HeartBeatSceneChange)
    getLogger().info("Installed all hooks!");
}