#pragma once

#include <optional>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/canbus_isotp/canbus_isotp_component.h"
#include "esphome/components/obd2_client/obd2_client_component.h"

namespace esphome {
namespace obd2 {

class OBD2ClientPIDSensor : public sensor::Sensor, public PollingComponent, public Parented<OBD2ClientComponent> {
public:
    virtual void update() override;

    void set_service(uint8_t service);

    void set_pid(uint8_t pid);

    void set_lambda(std::function<std::optional<float>(std::vector<uint8_t>)>&& callback);

protected:
    uint8_t service_;

    uint8_t pid_;

    std::function<std::optional<float>(std::vector<uint8_t>)> callback_;

};

}
}
