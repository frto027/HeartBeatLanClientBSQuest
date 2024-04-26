#pragma once

#include "config-utils/shared/config-utils.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector3.hpp"
#include <string>

// Declare the mod config as "ModConfiguration" and declare all its values and functions.
DECLARE_CONFIG(ModConfig,
    // Declare "VariableA"
    //CONFIG_VALUE(VariableA, std::string, "Variable Name", "Variable Value");

    CONFIG_VALUE(Enabled, bool, "Enabled", true);

    CONFIG_VALUE(SelectedBleMac, std::string, "Selected BLE Device Mac", "");
    CONFIG_VALUE(HeartTextColor, UnityEngine::Color, "The Text Color", UnityEngine::Color(0.08,0.33,0.94,1));
    CONFIG_VALUE(HeartGameCorePos, UnityEngine::Vector3, "Game playing position", UnityEngine::Vector3(3.1,0.2,6.8));
    CONFIG_VALUE(HeartGameCoreRot, float, "Game playing rotate Y", 0);
    CONFIG_VALUE(HeartMainMenuPos, UnityEngine::Vector3, "Main menu position", UnityEngine::Vector3(-2.3,2.8,4));
    CONFIG_VALUE(HeartMainMenuRot, float, "Main menu rotate Y", -30);
    CONFIG_VALUE(HeartLineSpaceDelta, float, "Line Space Delta", -35);

    CONFIG_VALUE(HeartDataComeFlashColor, UnityEngine::Color, "Text Color when Data Flash", UnityEngine::Color(0,1,0.6,1));
    CONFIG_VALUE(HeartDataComeFlashDuration, float, "Speed when Data Flash", 1);

    CONFIG_VALUE(ModLang, std::string, "Mod Language", "auto");
);
