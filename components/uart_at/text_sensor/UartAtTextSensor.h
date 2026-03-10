#pragma once

#include "esphome/components/text_sensor/text_sensor.h"

#include "esphome/components/uart_at/UartAtBaseSensor.h"

namespace esphome {
namespace uart_at {

    class UartAtTextSensor : public UartAtBaseSensor<std::string>
                                , public text_sensor::TextSensor {

    protected:
        void publish_state_(const std::string& state) override {
            publish_state(state);
        }
    };
}
}
