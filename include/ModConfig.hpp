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

    CONFIG_VALUE(HeartDataComeFlashColor, UnityEngine::Color, "Text Color when Data Flash", UnityEngine::Color(0,1,0.6,1));
    CONFIG_VALUE(HeartReplayDataComeFlashColor, UnityEngine::Color, "Text Color when Replay Data Flash", UnityEngine::Color(250./255., 224./255., 52./255.,1));
    CONFIG_VALUE(HeartDataComeFlashDuration, float, "Speed when Data Flash", 1);

    CONFIG_VALUE(ModLang, std::string, "Mod Language", "auto");

    CONFIG_VALUE(DisplayEnergy, bool, "Display Energy", true);

    CONFIG_VALUE(DataSourceType, int, "DataSourceType", 2);

    CONFIG_VALUE(OSCPort, int, "OSC Port", 9000);
);
