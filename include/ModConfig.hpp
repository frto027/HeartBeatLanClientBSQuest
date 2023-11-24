#pragma once

#include "config-utils/shared/config-utils.hpp"

// Declare the mod config as "ModConfiguration" and declare all its values and functions.
DECLARE_CONFIG(ModConfig,
    // Declare "VariableA"
    //CONFIG_VALUE(VariableA, std::string, "Variable Name", "Variable Value");
    CONFIG_VALUE(SelectedBleMac, std::string, "Selected BLE Device Mac", "");

)
