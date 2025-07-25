#pragma once

#include "config-utils/shared/config-utils.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector3.hpp"
#include <string>

// Declare the mod config as "ModConfiguration" and declare all its values and functions.

#if defined(GAME_VER_1_28_0) || defined(GAME_VER_1_35_0) || defined(GAME_VER_1_37_0)
DECLARE_CONFIG(ModConfig,
#else
DECLARE_CONFIG(ModConfig){
#endif
    // Declare "VariableA"
    //CONFIG_VALUE(VariableA, std::string, "Variable Name", "Variable Value");

    CONFIG_VALUE(Enabled, bool, "Enabled", true);

    CONFIG_VALUE(EnableRecord, bool, "Enable Record", true);
    CONFIG_VALUE(RecordDevName, bool, "Record Device Name", true);

    CONFIG_VALUE(SelectedBleMac, std::string, "Selected BLE Device Mac", "");

    CONFIG_VALUE(SelectedUI, std::string, "SelectedUIBundle", "Default");

    CONFIG_VALUE(ModLang, std::string, "Mod Language", "auto");

    CONFIG_VALUE(DisplayEnergy, bool, "Display Energy", true);

    CONFIG_VALUE(Age, int, "Age", 25);
    CONFIG_VALUE(MaxHeart, int, "MaxHeart", 220 - 25);

    CONFIG_VALUE(DataSourceType, int, "DataSourceType", 2);

    CONFIG_VALUE(OSCPort, int, "OSC Port", 9000);

    CONFIG_VALUE(HypeRateId, std::string, "HypeRateID", "")

    CONFIG_VALUE(HypeRateWebSocketIdentity, std::string, "HypeRateWebsocketIdentity", "")
#if defined(GAME_VER_1_28_0) || defined(GAME_VER_1_35_0) || defined(GAME_VER_1_37_0)
);
#else
};
#endif
