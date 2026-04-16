#include "obd2_client_component.h"

#include "esphome/core/hal.h"

namespace esphome {
namespace obd2 {

static const char *const TAG = "obd2_client";

void OBD2ClientComponent::setup() {
    for (auto canbus_isotp : canbus_isotp_ecus_) {
        canbus_isotp->add_callback([this, canbus_isotp](uint32_t can_id, const std::vector<uint8_t> &data) {
            if (can_id < 0x7E8 || can_id > 0x7EF) {
                return; // Not OBD2
            }
            if (!requests_.empty()) {
                auto& request = requests_.front();
                if (request.status == REQUEST_STATUS_WAITING_REPLY) {
                    // TODO check if service/PID match?
                    request.callback(canbus_isotp, can_id, data);
                    requests_.pop();
                }
            }
        });
    }
}

void OBD2ClientComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "obd2_client:");
    ESP_LOGCONFIG(TAG, "  ECU count: %i", canbus_isotp_ecus_.size());
}

void OBD2ClientComponent::loop() {
    if (!requests_.empty()) {
        auto& request = requests_.front();
        if (request.status == REQUEST_STATUS_PENDING) {
            std::vector<uint8_t> full_data;
            full_data.reserve(request.data.size() + 2);
            full_data.push_back(request.service);
            if (request.pid) {
                full_data.push_back(*request.pid);
            }
            full_data.insert(full_data.end(), request.data.begin(), request.data.end());
            request.status = REQUEST_STATUS_WAITING_REPLY;
            request.timestamp = millis();
            this->canbus_isotp_broadcast_->send_data(full_data);
        } else if (request.status == REQUEST_STATUS_WAITING_REPLY) {
            if (request.timestamp + 1000 < millis()) {
                ESP_LOGW(TAG, "Request timed out: service=%i pid=%i", request.service, request.pid ? *request.pid : -1);
                // TODO execute callback w/ error signaling?
                requests_.pop();
            }
        }
    }
}

void OBD2ClientComponent::add_on_pid_callback(PIDCallback &&callback) {
    this->pid_callback.add(std::move(callback));
}

void OBD2ClientComponent::add_on_dtc_callback(DTCCallback &&callback) {
    this->dtc_callback.add(std::move(callback));
}

void OBD2ClientComponent::add_on_dtc_cleared_callback(DTCClearedCallback &&callback) {
    this->dtc_cleared_callback.add(std::move(callback));
}

void OBD2ClientComponent::set_canbus_isotp_broadcast(canbus_isotp::CanbusISOTPComponent* canbus_isotp) {
    this->canbus_isotp_broadcast_ = canbus_isotp;
}

void OBD2ClientComponent::add_canbus_isotp_ecu(canbus_isotp::CanbusISOTPComponent* canbus_isotp) {
    this->canbus_isotp_ecus_.push_back(canbus_isotp);
}

bool OBD2ClientComponent::readPID(OBD2Service service, uint8_t pid, PIDCallback &&callback) {
    return this->request_obd2(service, pid, {}, [=, this](canbus_isotp::CanbusISOTPComponent*, uint32_t can_id, const std::vector<uint8_t>& data){
        if (data.size() >= 2) {
            const std::vector<uint8_t> sub_data(data.begin() + 2, data.end());
            pid_callback(service, pid, sub_data);
            callback(service, pid, sub_data);
        }
        // TODO execute callback in case of error?
    });
}

bool OBD2ClientComponent::readDTCs(OBD2Service service, DTCCallback &&callback) {
    return this->request_obd2(service, {}, {}, [=, this](canbus_isotp::CanbusISOTPComponent*, uint32_t can_id, const std::vector<uint8_t>& data){
        std::vector<uint16_t> dtcs;
        for (size_t i = 2; i + 1 < data.size(); i += 2) {
            dtcs.push_back(data[i] << 8 | data[i+1]);
        }
        dtc_callback(service, dtcs);
        callback(service, dtcs);
    });
}

bool OBD2ClientComponent::clearDTCs(DTCClearedCallback &&callback) {
    return this->request_obd2(SERVICE_CLEAR_DTC, {}, {}, [=, this](canbus_isotp::CanbusISOTPComponent*, uint32_t can_id, const std::vector<uint8_t>& data){
        dtc_cleared_callback();
        callback();
    });
}

bool OBD2ClientComponent::request_obd2(uint8_t service, std::optional<uint8_t> pid, const std::vector<uint8_t> data, OBD2RequestCallback&& callback) {
    if (requests_.size() >= 10) {
        ESP_LOGE(TAG, "Request queue is full");
        return false;
    }
    requests_.push(OBD2Request{
        .status = REQUEST_STATUS_PENDING,
        .service = service,
        .pid = pid,
        .data = data,
        .callback = std::move(callback)
    });
    return true;
}

}
}
