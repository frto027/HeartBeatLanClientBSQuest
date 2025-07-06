#include "main.hpp"
#include "GlobalNamespace/CoreGameHUDController.hpp"
#include "HeartBeat.hpp"
#include "HeartBeatDataSource.hpp"
#include "HeartBeatSetthings.hpp"
#include "GlobalNamespace/CoreGameHUDController.hpp"
#include "UnityEngine/GameObject.hpp"
#include "bsml/shared/BSML.hpp"
#include "multi_version_compat.hpp"

#include "HMUI/HierarchyManager.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/MonoBehaviour.hpp"

#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"

#include "UnityEngine/Quaternion.hpp"

#include "custom-types/shared/macros.hpp"

#include "ModConfig.hpp"

#include "i18n.hpp"
#include <cstddef>
#include "BeatLeaderRecorder.hpp"

static modloader::ModInfo modInfo = {MOD_ID, VERSION, 0}; // Stores the ID and version of our mod, and is sent to the modloader upon startup

bool ModEnabled;

// Called at the early stages of game loading
extern "C" void setup(CModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo.assign(info);
	
    getModConfig().Init(modInfo);
    getLogger().info("Completed setup!");

    ModEnabled = getModConfig().Enabled.GetValue();

    if(ModEnabled){
        HeartBeat::DataSource::getInstance();
    }
}

Paper::ConstLoggerContext<21> & getLogger(){
    static Paper::ConstLoggerContext<21> logger = Paper::ConstLoggerContext("HeartBeatLanReceiver");
    return logger;
}
UnityEngine::GameObject* MainMenuPreviewObject = nullptr;
HeartBeat::HeartBeatObj *MainMenuPreviewObjectComp = nullptr;
MAKE_HOOK_MATCH(GameplayCoreHook, &GlobalNamespace::CoreGameHUDController::Initialize, void, GlobalNamespace::CoreGameHUDController * self, GlobalNamespace::CoreGameHUDController::InitData * data){
    GameplayCoreHook(self, data);
    if(MainMenuPreviewObject)
        MainMenuPreviewObject->set_active(false);

    HeartBeat::assetBundleMgr.Init();

    std::string SelectedUI = getModConfig().SelectedUI.GetValue();
    if(!HeartBeat::assetBundleMgr.loadedBundles.contains(SelectedUI))
        SelectedUI = "Default";
    if(!HeartBeat::assetBundleMgr.loadedBundles.contains(SelectedUI)){
        getLogger().error("Can't find ui asset bundle '{}' to load!", SelectedUI);
        return;
    }

    getLogger().info("Loading '{}' at game start", SelectedUI);

    UnityEngine::GameObject * parent = self->get_energyPanelGo();
    auto & assetUI = HeartBeat::assetBundleMgr.loadedBundles[SelectedUI];
    if(assetUI.infos.contains("root")){
        std::string root_str = assetUI.infos["root"];
        if(root_str == "energyPanelGo") parent = self->get_energyPanelGo();
        else if(root_str == "songProgressPanelGO") parent = self->get_songProgressPanelGO();
        else if(root_str == "relativeScoreGo") parent = self->get_relativeScoreGo();
        else if(root_str == "immediateRankGo") parent = self->get_immediateRankGo();
        else getLogger().info("unknown position {}, attach it to energyPanelGo", root_str);
    }
    getLogger().info("UI Mount position: {}", parent->get_name());

    HeartBeat::AssetBundleInstinateInformation result;
    if(!HeartBeat::assetBundleMgr.Instantiate(SelectedUI, parent->get_transform(), result)){
        getLogger().error("The UI Can't loaded.");
        return;
    }
    result.gameObject->AddComponent<HeartBeat::HeartBeatObj*>()->loadedComponents = result;
    getLogger().info("The UI has been created");
    static int firstInitialize = true;
    if(firstInitialize){
        firstInitialize = false;
        if(HeartBeat::dataSourceType == HeartBeat::DS_BLE){
            HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatBleDataSource>()->SetSelectedBleMac(getModConfig().SelectedBleMac.GetValue());
        }
    }
}
// Called later on in the game loading - a good time to install function hooks
extern "C" void late_load() {
    getLogger().info("Loading HeartbeatLan");

    il2cpp_functions::Init();

    getLogger().info("init BSML");
    BSML::Init();

    getLogger().info("init i18n");
    I18N::Setup();

    getLogger().info("Installing ui...");
    SetthingUI::Setup();

    if(ModEnabled == false){
        getLogger().info("The mod is not enabled");
        return;
    }


    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), GameplayCoreHook);

    getLogger().info("init recorder...");
    HeartBeat::Recorder::Init();

    getLogger().info("Done.");
}