#pragma once

#include <optional>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/canbus_isotp/canbus_isotp_component.h"
#include "esphome/components/obd2_client/obd2_client_component.h"

namespace esphome {
namespace obd2 {

class OBD2ClientDTCTextSensor : public text_sensor::TextSensor, public PollingComponent, public Parented<OBD2ClientComponent> {
public:
    virtual void update() override;

    void set_service(uint8_t service);

protected:
    uint8_t service_;

};

}
}
