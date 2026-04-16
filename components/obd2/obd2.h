#pragma once

#include <cinttypes>
#include <optional>
#include <string>

namespace esphome {
namespace obd2 {

enum OBD2Service {
    SERVICE_CURRENT_DATA = 0x01,
    SERVICE_FROZEN_DATA = 0x02,
    SERVICE_DTC_STORED = 0x03,
    SERVICE_CLEAR_DTC = 0x04,
    SERVICE_TEST_RESULTS_NON_CAN = 0x05,
    SERVICE_TEST_RESULTS_CAN = 0x06,
    SERVICE_DTC_PENDING = 0x07,
    SERVICE_CONTROL_ONBOARD_SYSTEM = 0x08,
    SERVICE_VEHICLE_INFO = 0x09,
    SERVICE_DTC_PERMANENT = 0x0A,
};

std::string dtc_to_string(uint16_t dtc);

}
}
