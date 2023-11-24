#pragma once

#include "config-utils/shared/config-utils.hpp"
#include "codegen/include/UnityEngine/Color.hpp"
#include "codegen/include/UnityEngine/Vector3.hpp"

// Declare the mod config as "ModConfiguration" and declare all its values and functions.
DECLARE_CONFIG(ModConfig,
    // Declare "VariableA"
    //CONFIG_VALUE(VariableA, std::string, "Variable Name", "Variable Value");
    CONFIG_VALUE(SelectedBleMac, std::string, "Selected BLE Device Mac", "");
    CONFIG_VALUE(HeartTextColor, UnityEngine::Color, "The Text Color", UnityEngine::Color(1,1,1,1));
    CONFIG_VALUE(HeartGameCorePos, UnityEngine::Vector3, "Game playing position", UnityEngine::Vector3(3.1,0.2,6.8));
    CONFIG_VALUE(HeartMainMenuPos, UnityEngine::Vector3, "Main menu position", UnityEngine::Vector3(-2.3,2.8,4));
);
