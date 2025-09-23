#include "dbc_sensor.h"

namespace esphome {
namespace dbc {

static const char *const TAG = "dbc";

void DBCSensor::on_frame(uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data, float signal_value) {
    if (signal_value != this->state || !this->has_state()) {
        publish_state(signal_value);
    }
}

}
}