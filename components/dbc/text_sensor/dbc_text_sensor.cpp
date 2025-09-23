#include "dbc_text_sensor.h"

namespace esphome {
namespace dbc {

static const char *const TAG = "dbc";

void DBCTextSensor::add_choice(float signal_value, const std::string& signal_value_str) {
    m_choices[signal_value] = signal_value_str;
}

void DBCTextSensor::on_frame(uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data, float signal_value) {
    std::string signal_value_str = "unknown";
    if (m_choices.contains(signal_value)) {
        signal_value_str = m_choices[signal_value];
    }
    if (signal_value_str != this->state || !this->has_state()) {
        publish_state(signal_value_str);
    }
}

}
}