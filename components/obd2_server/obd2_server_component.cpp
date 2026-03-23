#include "obd2_server_component.h"
#include "esphome/core/application.h"

namespace esphome {
namespace obd2 {

static const char *const TAG = "obd2_server";

#define OBD2_SERVER_SENSOR_IS_PRESENT_(name) (OBD2_SERVER_SENSOR_NAME_(name) != nullptr)
#define OBD2_SERVER_SENSOR_STATE_(name, default_) \
    (OBD2_SERVER_SENSOR_IS_PRESENT_(name) ? \
        OBD2_SERVER_SENSOR_NAME_(name)->state : \
        default_ \
    )

#ifdef USE_BINARY_SENSOR
#define OBD2_SERVER_BINARY_SENSOR_IS_PRESENT(name)
#define OBD2_SERVER_BINARY_SENSOR_STATE(name, default_) OBD2_SERVER_SENSOR_STATE_(name, default_)
#else
#define OBD2_SERVER_BINARY_SENSOR_IS_PRESENT(name) false
#define OBD2_SERVER_BINARY_SENSOR_STATE(name, default_) default_
#endif

#ifdef USE_SENSOR
#define OBD2_SERVER_SENSOR_IS_PRESENT(name)
#define OBD2_SERVER_SENSOR_STATE(name, default_) OBD2_SERVER_SENSOR_STATE_(name, default_)
#else
#define OBD2_SERVER_SENSOR_IS_PRESENT(name) false
#define OBD2_SERVER_SENSOR_STATE(name, default_) default_
#endif

#define OBD2_SERVER_BINARY_SENSOR_ENCODE_STATE(name, value, bit, default_) \
    if (OBD2_SERVER_BINARY_SENSOR_STATE(name, default_)) { \
        OBD2_SERVER_SET_MSB_BIT(value, bit); \
    }

#define OBD2_SERVER_SET_MSB_BIT(value, bit) value |= (1 << (31 - bit))

static const std::vector<uint8_t> encode_value(uint32_t value) {
    return {
        (uint8_t)((value & 0xFF000000) >> 24),
        (uint8_t)((value & 0xFF0000) >> 16),
        (uint8_t)((value & 0xFF00) >> 8),
        (uint8_t)(value & 0xFF)
    };
}

void OBD2ServerComponent::setup() {

    for (auto canbus_isotp : canbus_isotp_) {
        canbus_isotp->add_callback([this](uint32_t can_id, const std::vector<uint8_t> &data){
            if (data.size() >= 1) {
                const auto& obd2_service = data[0];
                if (obd2_service == 0x01 && data.size() >= 2) {
                    const auto& obd2_pid = data[1];
                    if (!is_pid_supported(obd2_service, obd2_pid)) {
                        return;
                    }
                    if (obd2_pid % 0x20 == 0) {
                        handle_supported_pids(can_id, data, obd2_service, obd2_pid);
                    } else if (obd2_pid == 0x01) {
                        uint32_t status = 0;
                        OBD2_SERVER_BINARY_SENSOR_ENCODE_STATE(mil_status, status, 0, false);
                        if (this->engine_type_) {
                            OBD2_SERVER_SET_MSB_BIT(status, 12);
                        }
                        #ifdef USE_BINARY_SENSOR
                        // Completeness
                        for (int i = 9; i <= 11; ++i) {
                            if (emission_tests_[i - 9] == nullptr || !emission_tests_[i - 9]->state) {
                                OBD2_SERVER_SET_MSB_BIT(status, i);
                            }
                        }
                        for (int i = 24; i <= 31; ++i) {
                            if (emission_tests_[i - 24 + 3] == nullptr || !emission_tests_[i - 24 + 3]->state) {
                                OBD2_SERVER_SET_MSB_BIT(status, i);
                            }
                        }
                        // Availability
                        for (int i = 13; i <= 15; ++i) {
                            if (emission_tests_[i - 13] != nullptr) {
                                OBD2_SERVER_SET_MSB_BIT(status, i);
                            }
                        }
                        for (int i = 16; i <= 23; ++i) {
                            if (emission_tests_[i - 16 + 3] != nullptr) {
                                OBD2_SERVER_SET_MSB_BIT(status, i);
                            }
                        }
                        #else
                        // Completeness
                        for (int i = 9; i <= 11; ++i) {
                            OBD2_SERVER_SET_MSB_BIT(status, i);
                        }
                        for (int i = 24; i <= 31; ++i) {
                            OBD2_SERVER_SET_MSB_BIT(status, i);
                        }
                        #endif
                        reply_obd2(obd2_service, obd2_pid, encode_value(status));
                    } else if (obd2_pid == 0x0C) {
                        reply_obd2(obd2_service, obd2_pid, {
                            (uint8_t)((int)(OBD2_SERVER_SENSOR_STATE(engine_speed, 0) * 4) / 256),
                            (uint8_t)((int)(OBD2_SERVER_SENSOR_STATE(engine_speed, 0) * 4) % 256)
                        });
                    }
                } else if (obd2_service == 0x09 && data.size() >= 2) {
                    const auto& obd2_pid = data[1];
                    if (!is_pid_supported(obd2_service, obd2_pid)) {
                        return;
                    }
                    if (obd2_pid % 0x20 == 0) {
                        handle_supported_pids(can_id, data, obd2_service, obd2_pid);
                    } else if (obd2_pid == 0x02) {
                        std::vector<uint8_t> encoded_str;
                        encoded_str.reserve(vin_.size() + 1);
                        encoded_str.push_back(0x01);
                        encoded_str.insert(encoded_str.end(), vin_.begin(), vin_.end());
                        reply_obd2(obd2_service, obd2_pid, encoded_str);
                    } else if (obd2_pid == 0x0A) {
                        const auto& name = App.get_name();
                        std::vector<uint8_t> encoded_str;
                        encoded_str.reserve(name.size() + 1);
                        encoded_str.push_back(0x01);
                        encoded_str.insert(encoded_str.end(), name.begin(), name.end());
                        reply_obd2(obd2_service, obd2_pid, encoded_str);
                    }
                }
            }
        });
    }
}

void OBD2ServerComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "obd2_server:");
}

void OBD2ServerComponent::loop() {
}

void OBD2ServerComponent::add_canbus_isotp(canbus_isotp::CanbusISOTPComponent* canbus_isotp) {
    canbus_isotp_.push_back(canbus_isotp);
}

bool OBD2ServerComponent::is_pid_supported(uint8_t obd2_service, uint8_t obd2_pid) const {
    const auto& sensors_per_pid = get_required_values_for_service(obd2_service);
    if (obd2_pid < sensors_per_pid.size()) {
        const auto& sensors = sensors_per_pid[obd2_pid];
        for (void* sensor_p : sensors) {
            if (sensor_p == nullptr) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

const std::vector<std::vector<void*> > OBD2ServerComponent::get_required_values_for_service(uint8_t service) const {
    if (service == 0x01 || service == 0x02) {
        return {
            {}, // 0x00 - PIDs supported [$01 - $20] 
            {   // 0x01 - Monitor status since DTCs cleared
                OBD2_SERVER_BINARY_SENSOR_POINTER(mil_status),
            },
            {    // 0x02  - DTC that caused freeze frame to be stored
                nullptr,
            },
            {    // 0x03  - Fuel system status
                nullptr,
            },
            {    // 0x04  - Calculated engine load
                nullptr,
            },
            {    // 0x05  -  Engine coolant temperature
                nullptr,
            },
            {    // 0x06  - Short term fuel trim (STFT)—Bank 1
                nullptr,
            },
            {    // 0x07  - Long term fuel trim (LTFT)—Bank 1
                nullptr,
            },
            {    // 0x08  - Short term fuel trim (STFT)—Bank 2
                nullptr,
            },
            {    // 0x09  - Long term fuel trim (LTFT)—Bank 2
                nullptr,
            },
            {    // 0x0A  - Fuel pressure
                nullptr,
            },
            {    // 0x0B  - Intake manifold absolute pressure
                nullptr,
            },
            {    // 0x0C  - Engine speed
                OBD2_SERVER_BINARY_SENSOR_POINTER(engine_speed),
            },
            {    // 0x0D  - Vehicle speed
                nullptr,
            },
        };
    } else if (service == 0x09) {
        return {
            {}, // 0x00 - PIDs supported [$01 - $20] 
            {   // 0x01 - VIN Message Count in PID 02
                nullptr, // N/A for CAN
            },
            {    // 0x02  - VIN
                vin_.length() == 0 ? nullptr : (void*)0x1,
            },
            {   // 0x03 - Calibration ID message count for PID 04
                nullptr, // N/A for CAN
            },
            {    // 0x04  - Calibration ID 
                nullptr,
            },
            {    // 0x05  - Calibration verification numbers (CVN) message count for PID 06
                nullptr, // N/A for CAN
            },
            {    // 0x06  - Calibration Verification Numbers (CVN)
                nullptr,
            },
            {    // 0x07  - In-use performance tracking message count for PID 08 and 0B
                nullptr, // N/A for CAN
            },
            {    // 0x08  - In-use performance tracking for spark ignition vehicles 
                nullptr,
            },
            {    // 0x09  - ECU name message count for PID 0A
                nullptr, // N/A for CAN
            },
            {    // 0x0A  - ECU name
                (void*)0x01,
            },
            {    // 0x0B - In-use performance tracking for compression ignition vehicles 
                nullptr,
            },
        };
    }
    return {};
}

bool OBD2ServerComponent::reply_obd2(uint8_t request_service, uint8_t request_pid, const std::vector<uint8_t> data) {
    std::vector<uint8_t> full_data;
    full_data.reserve(data.size() + 2);
    full_data.push_back(request_service + 0x40);
    full_data.push_back(request_pid);
    full_data.insert(full_data.end(), data.begin(), data.end());
    return canbus_isotp_[0]->send_data(full_data);
}

bool OBD2ServerComponent::handle_supported_pids(uint32_t can_id, const std::vector<uint8_t> &data, uint8_t obd2_service, uint8_t obd2_pid) {
    if (obd2_pid % 0x20 != 0) {
        return false;
    }
    uint32_t supported_pids = 0;

    for (int test_pid = obd2_pid + 1; test_pid <= obd2_pid + 0x20; ++test_pid) {
        if (is_pid_supported(obd2_service, test_pid)) {
            supported_pids |= (1 << (0x20 - test_pid - obd2_pid));
        }
    }

    return reply_obd2(obd2_service, obd2_pid, {
        (uint8_t)((supported_pids & 0xFF000000) >> 24),
        (uint8_t)((supported_pids & 0xFF0000) >> 16),
        (uint8_t)((supported_pids & 0xFF00) >> 8),
        (uint8_t)(supported_pids & 0xFF)
    });
}

}
}
