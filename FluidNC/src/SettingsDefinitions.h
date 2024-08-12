#pragma once

#include "Settings.h"

extern StringSetting* config_filename;

extern StringSetting* build_info;

extern StringSetting* start_message;

extern IntSetting* status_mask;

extern IntSetting* sd_fallback_cs;

extern EnumSetting* message_level;

extern EnumSetting* gcode_echo;

// Current tool persistance modification
extern IntSetting* current_tool;

// Current tool length offset persistance modification
extern StringSetting* current_tlo;