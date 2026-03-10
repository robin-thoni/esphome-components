#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"

#include "esphome/components/uart_at/UartAtBaseSensor.h"

namespace esphome {
namespace uart_at {

    class UartAtBinarySensor : public UartAtBaseSensor<bool>
                                , public binary_sensor::BinarySensor {

    protected:
        void publish_state_(const bool& state) override {
            publish_state(state);
        }
    };
}
}
