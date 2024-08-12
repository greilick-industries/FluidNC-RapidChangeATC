#include "RapidChange.h"
#include "../GCode.h"
#include "../Protocol.h"

namespace RapidChange {
    EnumItem directions[] = {
        { RapidChange::POSITIVE, "POSITIVE" },
        { RapidChange::NEGATIVE, "NEGATIVE" },
        EnumItem(RapidChange::POSITIVE)
    };
    EnumItem alignments[] = {
        { RapidChange::X_AXIS, "X_AXIS" },
        { RapidChange::Y_AXIS, "Y_AXIS" },
        EnumItem(RapidChange::X_AXIS)
    };
    // Dust cover control on open axis
    EnumItem dustCoverAxes[] = {
        { RapidChange::A_AXIS, "A_AXIS" },
        { RapidChange::B_AXIS, "B_AXIS" },
        { RapidChange::B_AXIS, "C_AXIS" },
        EnumItem(RapidChange::A_AXIS)
    };

    float RapidChange::calculate_tool_pos(uint8_t axis, uint8_t tool_num, float ref_value) {
        int multiplier = this->_direction == POSITIVE ? 1 : -1;
        if (axis == this->_alignment) {
            return ref_value + (tool_num - 1) * this->_pocket_offset * multiplier;
        } else {
            return ref_value;
        }
    }

    float RapidChange::get_manual_pos(uint8_t axis) {
        if (axis == X_AXIS) {
            return this->_x_tool_setter;
        } else {
            return this->_y_tool_setter;
        }
    }

    float RapidChange::get_touch_probe_pos(uint8_t axis) {
        if (axis == X_AXIS) {
            return this->_x_tool_setter;
        } else {
            return this->_y_tool_setter;
        }
    }

    float RapidChange::get_tool_pos(uint8_t axis, uint8_t tool_num) {
        if (!tool_has_pocket(tool_num)) {
            return get_manual_pos(axis);
        } else if (axis == X_AXIS) {        
            return calculate_tool_pos(axis, tool_num, this->_x_pocket_1);
        } else {
            return calculate_tool_pos(axis, tool_num, this->_y_pocket_1);
        }
    }

    bool RapidChange::tool_has_pocket(uint8_t tool_num) {
        return tool_num != 0 && tool_num <= this->_number_of_pockets;
    }

    void RapidChange::group(Configuration::HandlerBase& handler) {
        // magazine pockets
        handler.item("alignment", _alignment, alignments);
        handler.item("direction", _direction, directions);
        handler.item("number_of_pockets", _number_of_pockets);
        handler.item("pocket_offset", _pocket_offset);

        // magazine references
        handler.item("x_pocket_1", _x_pocket_1);
        handler.item("y_pocket_1", _y_pocket_1);
        handler.item("z_engage", _z_engage);
        handler.item("z_traverse", _z_traverse);
        handler.item("z_safe_clearance", _z_safe_clearance); 

        // speeds and feeds
        handler.item("engage_feed_rate", _engage_feed_rate);
        handler.item("load_rpm", _load_rpm);
        handler.item("unload_rpm", _unload_rpm);

        // enabled options
        handler.item("dust_cover_enabled", _dust_cover_enabled);
        handler.item("tool_recognition_enabled", _tool_recognition_enabled);
        handler.item("tool_setter_enabled", _tool_setter_enabled);

        // tool setter references
        handler.item("x_tool_setter", _x_tool_setter);
        handler.item("y_tool_setter", _y_tool_setter); 
        handler.item("z_seek_start", _z_seek_start);
        handler.item("z_safe_tool_setter", _z_safe_tool_setter);

        // tool touch off
        handler.item("set_tool_offset", _set_tool_offset);
        handler.item("seek_feed_rate", _seek_feed_rate);
        handler.item("seek_retreat", _seek_retreat);
        handler.item("set_feed_rate", _set_feed_rate);
        handler.item("set_tool_max_travel", _set_tool_max_travel);   

        // dust cover
        handler.item("dust_cover_axis", _dust_cover_axis, dustCoverAxes);
        handler.item("dust_cover_open_position", _dust_cover_open_position);
        handler.item("dust_cover_closed_position", _dust_cover_closed_position);
        handler.item("dust_cover_use_output", _dust_cover_use_output);
        handler.item("dust_cover_pin", _dust_cover_pin);

        // tool recognition
        handler.item("z_tool_recognition_zone_1", _z_tool_recognition_zone_1);
        handler.item("z_tool_recognition_zone_2", _z_tool_recognition_zone_2);
        handler.item("tool_recognition_pin", _tool_recognition_pin);
    }

    void RapidChange::afterParse() {
        this->_dust_cover_pin.setAttr(Pin::Attr::Output);
        this->_tool_recognition_pin.setAttr(Pin::Attr::Input);
    }
}