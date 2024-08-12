#include "../GCode.h"
#include "../Machine/MachineConfig.h"
#include "../Protocol.h"
#include "../Serial.h"
#include "../Settings.h"
#include "../SettingsDefinitions.h"
#include <stdarg.h>

struct StoredState {
    CoolantState coolant;
    Distance distance;
    FeedRate feed_rate_mode;
    Units units;
};

enum SpinState {
    CW = 3,
    CCW = 4,
    STOP = 5
};

// uint32_t current_tool = 0;
StoredState stored_state;
RapidChange::RapidChange* rapid_change;
// float origin_mpos[2];

void spin_cw(int speed);
void spin_ccw(int speed);
void spin_stop();
void execute_linef(bool sync_after, const char* format, ...);
void rapid_to_tool_setter_xy();
void rapid_to_pocket_xy(uint32_t tool_num);
void rapid_to_z(float position);
void linear_to_z(float position, int feedrate);
void message_start();
void open_dust_cover(bool open);
void open_dust_cover_axis(bool open);
void open_dust_cover_output(bool open);
void load_tool(uint32_t tool_num);
void set_tlo();
float get_current_tlo();
Error set_current_tlo(float tlo);
Error set_current_tool(uint8_t tool_num);
// void set_tool();
void record_program_state();
void restore_program_state();
void set_tool_change_state();
void unload_tool();

bool spindle_has_tool();