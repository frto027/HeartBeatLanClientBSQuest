#include "main.hpp"
#include "GlobalNamespace/CoreGameHUDController.hpp"
#include "HeartBeat.hpp"
#include "HeartBeatDataSource.hpp"
#include "HeartBeatSetthings.hpp"
#include "GlobalNamespace/CoreGameHUDController.hpp"
#include "UnityEngine/GameObject.hpp"
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
#include <cstddef>

static modloader::ModInfo modInfo = {MOD_ID, VERSION, 0}; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Called at the early stages of game loading
extern "C" void setup(CModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo.assign(info);
	
    getModConfig().Init(modInfo);
    getLogger().info("Completed setup!");

    if(getModConfig().Enabled.GetValue()){
        HeartBeat::DataSource::getInstance();
    }
}

Paper::ConstLoggerContext<21> & getLogger(){
    static Paper::ConstLoggerContext<21> logger = Paper::ConstLoggerContext("HeartBeatLanReceiver");
    return logger;
}
UnityEngine::GameObject * MainMenuPreviewObject = nullptr;
MAKE_HOOK_MATCH(GameplayCoreHook, &GlobalNamespace::CoreGameHUDController::Initialize, void, GlobalNamespace::CoreGameHUDController * self, GlobalNamespace::CoreGameHUDController::InitData * data){
    GameplayCoreHook(self, data);

    if(MainMenuPreviewObject)
        MainMenuPreviewObject->set_active(false);

    UnityEngine::GameObject * EnergyGo = self->get_energyPanelGo();
    auto text = BSML::Lite::CreateText(EnergyGo->get_transform(), "");
    auto rect = text->get_rectTransform();
    rect->SetParent(EnergyGo->transform, false);
    rect->anchoredPosition = {0.5, 0.5};
    rect->sizeDelta = {180, 20};

    text->color = getModConfig().HeartTextColor.GetValue();
    text->fontSize = 10;
    text->set_alignment(TMPro::TextAlignmentOptions::MidlineLeft);
    text->get_gameObject()->AddComponent<HeartBeat::HeartBeatObj*>();
}
bool ModEnabled;
// Called later on in the game loading - a good time to install function hooks
extern "C" void late_load() {
    getLogger().info("Loading HeartbeatLan");

    il2cpp_functions::Init();

    ModEnabled = getModConfig().Enabled.GetValue();
    HeartBeat::dataSourceType = (HeartBeat::DataSourceType)getModConfig().DataSourceType.GetValue();

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

    getLogger().info("Installed all hooks!");
}