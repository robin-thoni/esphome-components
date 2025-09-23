#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/dbc/dbc_base_sensor.h"

namespace esphome {
namespace dbc {

class DBCTextSensor : public DBCBaseSensor, public text_sensor::TextSensor {
    public:
        void add_choice(float signal_value, const std::string& signal_value_str);
    protected:
        std::map<float, std::string> m_choices;

        virtual void on_frame(uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data, float signal_value) override;
};
}
}