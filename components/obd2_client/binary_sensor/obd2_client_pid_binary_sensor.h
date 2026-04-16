#pragma once

#include <optional>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/canbus_isotp/canbus_isotp_component.h"
#include "esphome/components/obd2_client/obd2_client_component.h"

namespace esphome {
namespace obd2 {

class OBD2ClientPIDBinarySensor : public binary_sensor::BinarySensor, public PollingComponent, public Parented<OBD2ClientComponent> {
public:
    virtual void update() override;

    void set_service(uint8_t service);

    void set_pid(uint8_t pid);

    void set_lambda(std::function<std::optional<bool>(std::vector<uint8_t>)>&& callback);

protected:
    uint8_t service_;

    uint8_t pid_;

    std::function<std::optional<bool>(std::vector<uint8_t>)> callback_;

};

}
}
