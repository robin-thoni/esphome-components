#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/dbc/dbc_base_sensor.h"

namespace esphome {
namespace dbc {

class DBCSensor : public DBCBaseSensor, public sensor::Sensor {
    protected:
        virtual void on_frame(uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data, float signal_value) override;
};
}
}