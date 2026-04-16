#pragma once

#include <optional>
#include <queue>

#include "esphome/core/component.h"
#include "esphome/components/canbus_isotp/canbus_isotp_component.h"
#include "esphome/components/obd2/obd2.h"

namespace esphome {
namespace obd2 {

typedef std::function<void(canbus_isotp::CanbusISOTPComponent*, uint32_t can_id, const std::vector<uint8_t>&)> OBD2RequestCallback;
typedef std::function<void(uint8_t, uint8_t, const std::vector<uint8_t>&)> PIDCallback;
typedef std::function<void(uint8_t, const std::vector<uint16_t>&)> DTCCallback;
typedef std::function<void()> DTCClearedCallback;

enum OBD2RequestStatus {
    REQUEST_STATUS_PENDING = 0,
    REQUEST_STATUS_WAITING_REPLY = 1,
};

struct OBD2Request {
    OBD2RequestStatus status;
    uint64_t timestamp;
    uint8_t service;
    std::optional<uint8_t> pid;
    std::vector<uint8_t> data;
    OBD2RequestCallback callback;
};

class OBD2ClientComponent : public Component {
public:
    void setup() override;
    void dump_config() override;
    void loop() override;

    void add_on_pid_callback(PIDCallback &&callback);

    void add_on_dtc_callback(DTCCallback &&callback);

    void add_on_dtc_cleared_callback(DTCClearedCallback &&callback);

    void set_canbus_isotp_broadcast(canbus_isotp::CanbusISOTPComponent* canbus_isotp);

    void add_canbus_isotp_ecu(canbus_isotp::CanbusISOTPComponent* canbus_isotp);

    bool readPID(OBD2Service service, uint8_t pid, PIDCallback &&callback);

    bool readDTCs(OBD2Service service, DTCCallback &&callback);

    bool clearDTCs(DTCClearedCallback &&callback);

    bool request_obd2(uint8_t service, std::optional<uint8_t> pid, const std::vector<uint8_t> data, OBD2RequestCallback&& callback);

protected:
    LazyCallbackManager<void(uint8_t service, uint8_t pid, const std::vector<uint8_t>& data)> pid_callback{};

    LazyCallbackManager<void(uint8_t service, const std::vector<uint16_t>& dtcs)> dtc_callback{};

    LazyCallbackManager<void()> dtc_cleared_callback{};

    std::queue<OBD2Request> requests_;

    canbus_isotp::CanbusISOTPComponent* canbus_isotp_broadcast_;

    std::vector<canbus_isotp::CanbusISOTPComponent*> canbus_isotp_ecus_;

};

}
}
