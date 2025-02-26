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

    CONFIG_VALUE(Age, int, "Player Age", 25);

    CONFIG_VALUE(SelectedBleMac, std::string, "Selected BLE Device Mac", "");
    CONFIG_VALUE(HeartTextColor, UnityEngine::Color, "The Text Color", UnityEngine::Color(0.08,0.33,0.94,1));

    CONFIG_VALUE(HeartDataComeFlashColor, UnityEngine::Color, "Text Color when Data Flash", UnityEngine::Color(0,1,0.6,1));
    CONFIG_VALUE(HeartDataComeFlashDuration, float, "Speed when Data Flash", 1);

    // CONFIG_VALUE(HeartTextColorZone0, UnityEngine::Color, "The Text Color Zone0", UnityEngine::Color(0.08,0.33,0.94,1)); // None                          0 -  97bpm
    // CONFIG_VALUE(HeartDataComeFlashColorZone0, UnityEngine::Color, "Text Color when Data Flash Zone0", UnityEngine::Color(0,1,0.6,1));
    // CONFIG_VALUE(HeartTextColorZone1, UnityEngine::Color, "The Text Color Zone1", UnityEngine::Color(0.08,0.33,0.94,1)); // Moderate-low, 50% - 60%   97bpm - 117bpm
    // CONFIG_VALUE(HeartDataComeFlashColorZone1, UnityEngine::Color, "Text Color when Data Flash Zone1", UnityEngine::Color(0,1,0.6,1));
    // CONFIG_VALUE(HeartTextColorZone2, UnityEngine::Color, "The Text Color Zone2", UnityEngine::Color(0.08,0.33,0.94,1)); // Moderate 60%-70%         117bpm - 136bpm
    // CONFIG_VALUE(HeartDataComeFlashColorZone2, UnityEngine::Color, "Text Color when Data Flash Zone2", UnityEngine::Color(0,1,0.6,1));
    // CONFIG_VALUE(HeartTextColorZone3, UnityEngine::Color, "The Text Color Zone3", UnityEngine::Color(0.08,0.33,0.94,1)); // Moderate-high 70%-80%    136bpm - 156bpm
    // CONFIG_VALUE(HeartDataComeFlashColorZone3, UnityEngine::Color, "Text Color when Data Flash Zone3", UnityEngine::Color(0,1,0.6,1));
    // CONFIG_VALUE(HeartTextColorZone4, UnityEngine::Color, "The Text Color Zone4", UnityEngine::Color(0.08,0.33,0.94,1)); // High  80%-90%            156bpm - 175bpm
    // CONFIG_VALUE(HeartDataComeFlashColorZone4, UnityEngine::Color, "Text Color when Data Flash Zone4", UnityEngine::Color(0,1,0.6,1));
    // CONFIG_VALUE(HeartTextColorZone5, UnityEngine::Color, "The Text Color Zone5", UnityEngine::Color(0.941, 0.075, 0.075,1)); // Very-high 90%-100%  175bpm - 195bpm
    // CONFIG_VALUE(HeartDataComeFlashColorZone5, UnityEngine::Color, "Text Color when Data Flash Zone5", UnityEngine::Color(0,1,0.6,1));


    CONFIG_VALUE(ModLang, std::string, "Mod Language", "auto");

    CONFIG_VALUE      (DisplayEnergy, bool, "Display Energy", true);

    CONFIG_VALUE(DataSourceType, int, "DataSourceType", 2);

    CONFIG_VALUE(OSCPort, int, "OSC Port", 9000);
);
