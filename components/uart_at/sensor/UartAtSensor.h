#pragma once

#include "esphome/components/sensor/sensor.h"

#include "esphome/components/uart_at/UartAtBaseSensor.h"

namespace esphome {
namespace uart_at {

    class UartAtSensor : public UartAtBaseSensor<float>
                        , public sensor::Sensor {
    protected:
        void publish_state_(const float& state) override {
            publish_state(state);
        }
    };
}
}
