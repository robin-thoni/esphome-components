#include "obd2_client_dtc_text_sensor.h"

namespace esphome {
namespace obd2 {

void OBD2ClientDTCTextSensor::update() {
    this->parent_->readDTCs((obd2::OBD2Service)this->service_, [this](uint8_t service, const std::vector<uint16_t>& dtcs) {
        auto sorted_dtcs = dtcs;
        std::sort(sorted_dtcs.begin(), sorted_dtcs.end());
        std::string dtcs_str;
        dtcs_str.reserve(6 * sorted_dtcs.size());
        for (auto i = 0; i < sorted_dtcs.size(); ++i) {
            dtcs_str += obd2::dtc_to_string(sorted_dtcs[i]);
            if (i < sorted_dtcs.size() - 1) {
                dtcs_str += ",";
            }
        }
        this->publish_state(dtcs_str);
    });
}

void OBD2ClientDTCTextSensor::set_service(uint8_t service) {
    this->service_ = service;
}

}
}
