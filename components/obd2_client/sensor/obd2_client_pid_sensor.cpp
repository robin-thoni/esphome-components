#include "obd2_client_pid_sensor.h"

namespace esphome {
namespace obd2 {

void OBD2ClientPIDSensor::update() {
    this->parent_->readPID((obd2::OBD2Service)this->service_, this->pid_, [this](uint8_t service, uint8_t pid, const std::vector<uint8_t>& data){
        std::optional<float> res;
        if (this->callback_) {
            res = this->callback_(data);
        } else if (data.size() >= 1) {
            res = (float)data[0];
        }
        if (res) {
            this->publish_state(*res);
        }
    });
}

void OBD2ClientPIDSensor::set_service(uint8_t service) {
    this->service_ = service;
}

void OBD2ClientPIDSensor::set_pid(uint8_t pid) {
    this->pid_ = pid;
}

void OBD2ClientPIDSensor::set_lambda(std::function<std::optional<float>(std::vector<uint8_t>)>&& callback) {
    this->callback_ = std::move(callback);
}

}
}
