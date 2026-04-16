#include "obd2_client_pid_binary_sensor.h"

namespace esphome {
namespace obd2 {

void OBD2ClientPIDBinarySensor::update() {
    this->parent_->readPID((obd2::OBD2Service)this->service_, this->pid_, [this](uint8_t service, uint8_t pid, const std::vector<uint8_t>& data){
        std::optional<bool> res;
        if (this->callback_) {
            res = this->callback_(data);
        } else if (data.size() >= 1) {
            res = (bool)(data[0] & 0x80);
        }
        if (res) {
            this->publish_state(*res);
        }
    });
}

void OBD2ClientPIDBinarySensor::set_service(uint8_t service) {
    this->service_ = service;
}

void OBD2ClientPIDBinarySensor::set_pid(uint8_t pid) {
    this->pid_ = pid;
}

void OBD2ClientPIDBinarySensor::set_lambda(std::function<std::optional<bool>(std::vector<uint8_t>)>&& callback) {
    this->callback_ = std::move(callback);
}

}
}
