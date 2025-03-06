#pragma once

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include "UnityEngine/GameObject.hpp"
#include "scotland2/shared/loader.hpp"

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

namespace HeartBeat{
class HeartBeatObj;
}
Paper::ConstLoggerContext<21> & getLogger();

extern bool ModEnabled;
extern UnityEngine::GameObject* MainMenuPreviewObject;
extern HeartBeat::HeartBeatObj *MainMenuPreviewObjectComp;
