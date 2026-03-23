#pragma once

#include "esphome/core/component.h"
#include "esphome/components/canbus_isotp/canbus_isotp_component.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

namespace esphome {
namespace obd2 {

#define OBD2_SERVER_CONCAT2(x,y) x##y
#define OBD2_SERVER_CONCAT(x,y) OBD2_SERVER_CONCAT2(x,y)
#define OBD2_SERVER_SENSOR_NAME_(name) OBD2_SERVER_CONCAT(name, _)
#define OBD2_SERVER_DEFINE_SENSOR_(type, name) \
    public: \
        void set_##name(type* sensor) { this->OBD2_SERVER_SENSOR_NAME_(name) = sensor; } \
    protected: \
        type* OBD2_SERVER_SENSOR_NAME_(name){nullptr}

#ifdef USE_BINARY_SENSOR
#define OBD2_SERVER_DEFINE_BINARY_SENSOR(name) OBD2_SERVER_DEFINE_SENSOR_(binary_sensor::BinarySensor, name)
#define OBD2_SERVER_BINARY_SENSOR_NAME(name) OBD2_SERVER_SENSOR_NAME_(name)
#define OBD2_SERVER_BINARY_SENSOR_POINTER(name) OBD2_SERVER_SENSOR_NAME_(name)
#else
#define OBD2_SERVER_DEFINE_BINARY_SENSOR(name)
#define OBD2_SERVER_BINARY_SENSOR_NAME(name)
#define OBD2_SERVER_BINARY_SENSOR_POINTER(name) nullptr
#endif

#ifdef USE_SENSOR
#define OBD2_SERVER_DEFINE_SENSOR(name) OBD2_SERVER_DEFINE_SENSOR_(sensor::Sensor, name)
#define OBD2_SERVER_SENSOR_NAME(name) OBD2_SERVER_SENSOR_NAME_(name)
#define OBD2_SERVER_SENSOR_POINTER(name) OBD2_SERVER_SENSOR_NAME_(name)
#else
#define OBD2_SERVER_DEFINE_SENSOR(name)
#define OBD2_SERVER_SENSOR_NAME(name)
#define OBD2_SERVER_SENSOR_POINTER(name) nullptr
#endif

#ifdef USE_SWITCH
#define OBD2_SERVER_DEFINE_SWITCH(name) OBD2_SERVER_DEFINE_SENSOR_(switch_::Switch, name)
#define OBD2_SERVER_SWITCH_NAME(name) OBD2_SERVER_SENSOR_NAME_(name)
#define OBD2_SERVER_SWITCH_POINTER(name) OBD2_SERVER_SENSOR_NAME_(name)
#else
#define OBD2_SERVER_DEFINE_SWITCH(name)
#define OBD2_SERVER_SWITCH_NAME(name)
#define OBD2_SERVER_SWITCH_POINTER(name) nullptr
#endif

class OBD2ServerComponent : public Component {
public:
    void setup() override;
    void dump_config() override;
    void loop() override;

    void add_canbus_isotp(canbus_isotp::CanbusISOTPComponent* canbus_isotp);

    bool is_pid_supported(uint8_t obd2_service, uint8_t obd2_pid) const;

    void set_engine_type(bool engine_type) { engine_type_ = engine_type; }

    void set_vin(const std::string& vin) { vin_ = vin; }

    #ifdef USE_BINARY_SENSOR
    void set_emission_test(uint8_t index, binary_sensor::BinarySensor* emission_test) { emission_tests_[index] = emission_test; }
    #endif

protected:
    const std::vector<std::vector<void*> > get_required_values_for_service(uint8_t service) const;

    bool reply_obd2(uint8_t request_service, uint8_t request_pid, const std::vector<uint8_t> data);

    bool handle_supported_pids(uint32_t can_id, const std::vector<uint8_t> &data, uint8_t obd2_service, uint8_t obd2_pid);

    std::vector<canbus_isotp::CanbusISOTPComponent*> canbus_isotp_;

    bool engine_type_{false};

    std::string vin_;

    OBD2_SERVER_DEFINE_BINARY_SENSOR(mil_status);

    OBD2_SERVER_DEFINE_SENSOR(engine_speed);

    OBD2_SERVER_DEFINE_SENSOR(vehicle_speed);

    #ifdef USE_BINARY_SENSOR
    binary_sensor::BinarySensor* emission_tests_[11]{nullptr};
    #endif

};
}
}
