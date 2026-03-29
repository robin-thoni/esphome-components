#pragma once

#include "esphome/core/automation.h"
#include "obd2_server_component.h"

namespace esphome {
namespace obd2 {

class DTCClearTrigger : public Trigger<> {
public:
    DTCClearTrigger(OBD2ServerComponent *comp) {
        comp->add_on_dtc_clear_callback([this]() { this->trigger(); });
    }
};

}
}
