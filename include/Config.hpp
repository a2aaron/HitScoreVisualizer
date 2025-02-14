#pragma once

#include "json/DefaultConfig.hpp"

DECLARE_CONFIG(GlobalConfig) {
    CONFIG_VALUE(ModEnabled, bool, "isEnabled", true);
    CONFIG_VALUE(SelectedConfig, std::string, "selectedConfig", "");
    CONFIG_VALUE(HideUntilDone, bool, "hideUntilCalculated", false);
    // not actually written to the config file
    HSV::Config CurrentConfig = defaultConfig;
};
