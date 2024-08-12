#pragma once

#include "../Configuration/Configurable.h"

#define NOT_ASSIGNED 0.0f

namespace RapidChange {
    class RapidChange : public Configuration::Configurable {
        private:           
            float calculate_tool_pos(uint8_t axis, uint8_t tool_num, float ref_value);
            float get_manual_pos(uint8_t axis);

        public:
            RapidChange() = default;

            // tool pockets
            int _alignment = X_AXIS;
            int _direction = POSITIVE;
            int _number_of_pockets;
            float _pocket_offset;

            // magazine references
            float _x_pocket_1;
            float _y_pocket_1;
            float _z_engage;
            float _z_traverse;
            float _z_safe_clearance;

            // speeds and feeds
            int _engage_feed_rate;
            int _load_rpm;
            int _unload_rpm;

            // enabled options
            bool _dust_cover_enabled;
            bool _tool_recognition_enabled;
            bool _tool_setter_enabled;

            // tool touch off
            float _set_tool_offset;
            int _seek_feed_rate;
            int _seek_retreat;
            int _set_feed_rate;
            float _set_tool_max_travel;

            // tool setter references
            float _x_tool_setter;
            float _y_tool_setter;
            float _z_seek_start;
            float _z_safe_tool_setter;

            // dust cover
            int _dust_cover_axis = 3;
            float _dust_cover_open_position;
            float _dust_cover_closed_position;
            bool _dust_cover_use_output;
            Pin _dust_cover_pin;

            // tool recognition
            float _z_tool_recognition_zone_1;
            float _z_tool_recognition_zone_2;
            Pin _tool_recognition_pin;

            enum dustCoverAxis {
                A_AXIS = 3,
                B_AXIS = 4,
                C_AXIS = 5
            };

            enum direction {
                NEGATIVE = 0,
                POSITIVE
            };
            enum alignment {
                X_AXIS = 0,
                Y_AXIS = 1
            };

            float get_touch_probe_pos(uint8_t axis);
            float get_tool_pos(uint8_t axis, uint8_t tool_num);
            bool tool_has_pocket(uint8_t tool_num);

            void group(Configuration::HandlerBase& handler) override;
            void afterParse() override;
            ~RapidChange() = default;
    };
}