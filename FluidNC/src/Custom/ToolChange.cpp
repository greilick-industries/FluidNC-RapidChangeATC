#include "ToolChange.h"

void user_select_tool(uint32_t new_tool) {
    set_current_tool(new_tool);
}

void user_tool_change(uint32_t new_tool) {
    if (current_tool->get() == new_tool) {
        log_info("CURRENT TOOL SELECTED. TOOL CHANGE BYPASSED.");
        return;
    }

    rapid_change = config->_rapidChange;

    message_start();
    protocol_buffer_synchronize(); 
    record_program_state();
    set_tool_change_state();
    unload_tool();
    load_tool(new_tool);
    set_tlo();
    open_dust_cover(false);

    // if (current_tool != 0) {
    //     execute_linef(true, "G53 G0 G90 X%5.3f Y%5.3f", origin_mpos[0], origin_mpos[1]);
    // }

    restore_program_state();

    rapid_change = nullptr;
}

void execute_linef(bool sync_after, const char* format, ...) {
    char gc_line[30];
    va_list args;
    va_start(args, format);
    int length = vsprintf(gc_line, format, args); 

    // remove
    log_info(gc_line);   

    gc_line[length] = '\n';
    gc_line[length + 1] = '\0';
    va_end(args);

    gc_execute_line(gc_line);

    if (sync_after) {
        protocol_buffer_synchronize();
    }
}

void rapid_to_tool_setter_xy() {
    float x_pos = rapid_change->get_touch_probe_pos(X_AXIS);
    float y_pos = rapid_change->get_touch_probe_pos(Y_AXIS);
    execute_linef(true, "G53 G0 X%5.3f Y%5.3f", x_pos, y_pos);
}

void rapid_to_pocket_xy(uint32_t tool_num) {
    float x_pos = rapid_change->get_tool_pos(X_AXIS, tool_num);
    float y_pos = rapid_change->get_tool_pos(Y_AXIS, tool_num);
    execute_linef(true, "G53 G0 X%5.3f Y%5.3f", x_pos, y_pos);
}

void rapid_to_z(float position) {
    execute_linef(true, "G53 G0 Z%5.3f", position);
}

void linear_to_z(float position, int feedrate) {
    execute_linef(false, "G53 G1 Z%5.3f F%d", position, feedrate);
}

void message_start() {
    char tool_msg[20];
    sprintf(tool_msg, "CURRENT TOOL: %d", current_tool->get());
    log_info(tool_msg);
    sprintf(tool_msg, "SELECTED TOOL: %d", gc_state.tool);
    log_info(tool_msg);
}

void open_dust_cover(bool open) {
    if (!rapid_change->_dust_cover_enabled) {
        return;
    } else if (rapid_change->_dust_cover_use_output) {
        open_dust_cover_output(open);
    } else {
        open_dust_cover_axis(open);
    }
}

void open_dust_cover_axis(bool open) {
    char letter;
    switch (rapid_change->_dust_cover_axis) {
        case RapidChange::RapidChange::B_AXIS:
            letter = 'B';
            break;
        case RapidChange::RapidChange::C_AXIS:
            letter = 'C';
            break;
        default:
            letter = 'A';
            break;
    }
    float position = open ? rapid_change->_dust_cover_open_position : rapid_change->_dust_cover_closed_position;
    execute_linef(true, "G53 G0 %c%5.3f", letter, position);
}

void open_dust_cover_output(bool open) {
    if (rapid_change->_dust_cover_pin.defined()) {
        rapid_change->_dust_cover_pin.synchronousWrite(open);
    }
}

void record_program_state() {
    stored_state.coolant = gc_state.modal.coolant;
    stored_state.distance = gc_state.modal.distance;
    stored_state.feed_rate_mode = gc_state.modal.feed_rate;
    stored_state.units = gc_state.modal.units;

    // float* current_mpos = get_mpos();
    // origin_mpos[0] = current_mpos[X_AXIS];
    // origin_mpos[1] = current_mpos[Y_AXIS];
}

void restore_program_state() {
    if (stored_state.coolant.Flood == 1) {
        execute_linef(false, "M8");
    } else if (stored_state.coolant.Mist == 1) {
        execute_linef(false, "M7");
    }
    // if (stored_state.distance == Distance::Incremental) {
    //     execute_linef(false, "G91");
    // }
    if (stored_state.feed_rate_mode == FeedRate::InverseTime) {
        execute_linef(false, "G93");
    }
    if (stored_state.units == Units::Inches) {
        execute_linef(true, "G20");
    }
}

void set_tool_change_state() {
    execute_linef(true, "M5 M9 G0 G21 G90 G94");
}

void set_tlo() {
    // If the tool setter is disabled or if we don't have a tool, rise up and be done
    if (!rapid_change->_tool_setter_enabled || current_tool->get() == 0) {
        rapid_to_z(rapid_change->_z_safe_clearance);
        return;
    }

    // Set the tool offset with a double probe, seek then set
    rapid_to_z(rapid_change->_z_safe_tool_setter);
    rapid_to_tool_setter_xy();
    rapid_to_z(rapid_change->_z_seek_start);
    execute_linef(true, "G38.2 G91 F%d Z%5.3f", rapid_change->_seek_feed_rate, -1 * rapid_change->_set_tool_max_travel);
    execute_linef(true, "G91 G0 Z%d", rapid_change->_seek_retreat);
    execute_linef(true, "G38.2 G91 F%d Z%5.3f", rapid_change->_set_feed_rate, -1 * (rapid_change->_seek_retreat + 2));

    float* m_pos = get_mpos();
    float m_pos_z = m_pos[Z_AXIS];
    float tlo = m_pos_z;

    Error err = set_current_tlo(tlo);
    char message[40];
    sprintf(message, "Saved/Fetched TLO: %s", current_tlo->get());
    log_info(message);

    execute_linef(false, "G43.1 Z%5.3f", tlo);

    //execute_linef(true, "G10 L20 P0 Z%5.3f", rapid_change->_set_tool_offset);
    execute_linef(true, "G90 G0");

    // And rise all the way because we are done
    rapid_to_z(rapid_change->_z_safe_clearance);
}

float get_current_tlo() {
    return atof(current_tlo->get());
}

Error set_current_tlo(float tlo) {
    char tlo_string[10];
    sprintf(tlo_string, "%5.3f", tlo);
    return current_tlo->setStringValue(tlo_string);
}

Error set_current_tool(uint8_t tool_num) {
    char tool_string[3];
    sprintf(tool_string, "%u", tool_num);
    Error err = current_tool->setStringValue(tool_string);
    // log_info(errorString(err));
    // char message[30];
    // sprintf(message, "Stored current tool: %s", tool_string);
    // log_info(message);
    // sprintf(message, "Fetched current tool: %u", current_tool->get());
    // log_info(message);
    return err;
}


void unload_tool() {
    rapid_to_z(rapid_change->_z_safe_clearance);

    // if we don't have a tool we're done
    if (current_tool->get() == 0) {
        open_dust_cover(true);
        return;
    }

    // If the tool has a pocket, unload
    if (rapid_change->tool_has_pocket(current_tool->get())) {

        // Perform first attempt
        rapid_to_pocket_xy(current_tool->get());
        open_dust_cover(true);
        execute_linef(false, "G4 P0.5");
        rapid_to_z(rapid_change->_z_engage + 23);
        spin_ccw(rapid_change->_unload_rpm);
        linear_to_z(rapid_change->_z_engage, rapid_change->_engage_feed_rate);

        // If we're using tool recognition, handle it
        if (rapid_change->_tool_recognition_enabled) {
            rapid_to_z(rapid_change->_z_tool_recognition_zone_1);

            // If we have a tool, try unloading one more time
            if (spindle_has_tool()) {
                rapid_to_z(rapid_change->_z_engage + 23);
                linear_to_z(rapid_change->_z_engage, rapid_change->_engage_feed_rate);
                rapid_to_z(rapid_change->_z_tool_recognition_zone_1);
            }

            // Whether successful or not, we're done trying
            spin_stop();

            // If we have a tool at this point, rise and pause for manual unloading

            if (spindle_has_tool()) {
                rapid_to_z(rapid_change->_z_safe_clearance);
                log_info("FAILED TO UNLOAD THE CURRENT TOOL.");
                log_info("PLEASE UNLOAD THE TOOL MANUALLY AND CYCLE START TO CONTINUE.");
                execute_linef(true, "M0");

            // Otherwise, get ready to unload
            } else {
                rapid_to_z(rapid_change->_z_traverse);
            }

        // If we're not using tool recognition, go straight to traverse height for loading    
        } else {
            rapid_to_z(rapid_change->_z_traverse);
            spin_stop();
        }

    // If the tool doesn't have a pocket, let's pause for manual removal
    } else {  
        open_dust_cover(true);
        log_info("CURRENT TOOL DOES NOT HAVE AN ASSIGNED POCKET.");
        log_info("PLEASE UNLOAD THE TOOL MANUALLY AND CYCLE START TO CONTINUE.");
        execute_linef(true, "M0");
    }

    // The tool has been removed, set current tool to 0
    set_current_tool(0);
}

void spin_cw(int speed) {
    execute_linef(false, "M3 S%d", speed);
    execute_linef(true, "G4 P1");
}

void spin_ccw(int speed) {
    execute_linef(false, "M4 S%d", speed);
    execute_linef(true, "G4 P1");
}

void spin_stop() {
    execute_linef(false, "M5");
}

void load_tool(uint32_t tool_num) {

    // If loading tool 0, we're done
    if (tool_num == 0) {
        return;
    }

    // If selected tool has a pocket, perform automatic pick up
    if (rapid_change->tool_has_pocket(tool_num)) {
        rapid_to_pocket_xy(tool_num);
        rapid_to_z(rapid_change->_z_engage + 23);
        spin_cw(rapid_change->_load_rpm);
        linear_to_z(rapid_change->_z_engage, rapid_change->_engage_feed_rate);
        rapid_to_z(rapid_change->_z_engage + 12);
        linear_to_z(rapid_change->_z_engage, rapid_change->_engage_feed_rate);

        // If we're using tool recognition, let's handle it
        if (rapid_change->_tool_recognition_enabled) {
            rapid_to_z(rapid_change->_z_tool_recognition_zone_1);
            spin_stop();

            // If we don't have a tool rise and pause for a manual load
            if (!spindle_has_tool()) {
                rapid_to_z(rapid_change->_z_safe_clearance);
                log_info("FAILED TO LOAD THE SELECTED TOOL.");
                log_info("PLEASE LOAD THE TOOL MANUALLY AND CYCLE START TO CONTINUE.");
                execute_linef(true, "M0");

            // Otherwise we have a tool and can perform the next check
            } else {
                rapid_to_z(rapid_change->_z_tool_recognition_zone_2);

                // If we show to have a tool here, we cross-threaded and need to manually load
                if (spindle_has_tool()) {
                    rapid_to_z(rapid_change->_z_safe_clearance);
                    log_info("FAILED TO PROPERLY THREAD THE SELECTED TOOL.");
                    log_info("PLEASE RELOAD THE TOOL MANUALLY AND CYCLE START TO CONTINUE.");
                    execute_linef(true, "M0");
                } // Otherwise all went well
            }

        // Otherwise we're not using tool recognition so let's get ready to set the tool offset
        } else {
            rapid_to_z(rapid_change->_z_traverse);
            spin_stop();
        }

    // Otherwise, there is no pocket so let's rise and pause to load manually
    } else {
        rapid_to_z(rapid_change->_z_safe_clearance);
        log_info("SELECTED TOOL DOES NOT HAVE AN ASSIGNED POCKET.");
        log_info("PLEASE LOAD THE SELECTED TOOL AND PRESS CYCLE START TO CONTINUE.");
        execute_linef(true, "M0");
    }

    // We've loaded our tool
    set_current_tool(tool_num);
}

bool spindle_has_tool() {
    return !rapid_change->_tool_recognition_pin.read();
}

void user_M30() {
    // TODO: call user_tool_change? this doesn't work
    // if (current_tool != 0) {
    //     execute_linef(true, "M6 T0");
    // }
}
