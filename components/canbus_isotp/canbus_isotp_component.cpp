#include "canbus_isotp_component.h"
#include "esphome/core/hal.h"
#ifdef USE_LOGGER
#include "esphome/components/logger/logger.h"
#endif

extern "C" {
int isotp_user_send_can(const uint32_t arbitration_id, const uint8_t* data, const uint8_t size, void* canbus_comp) {
    std::vector<uint8_t> data_vector;
    data_vector.assign(data, data + size);
    const auto& err = ((esphome::canbus::Canbus*)canbus_comp)->send_data(arbitration_id, false, data_vector);
    return err == esphome::canbus::ERROR_OK ? ISOTP_RET_OK : ISOTP_RET_ERROR;
}

uint32_t isotp_user_get_us(void) {
    return esphome::micros();
}

void isotp_user_debug(const char* message, ...) {
    #ifdef USE_LOGGER
    auto *log = esphome::logger::global_logger;
    if (log == nullptr) {
        return;
    }
    va_list arg;
    va_start(arg, message);
    log->log_vprintf_(ESPHOME_LOG_LEVEL_DEBUG, "canbus_isotp.isotp-c", 0, message, arg);
    va_end(arg);
    #endif
}

}

namespace esphome {
namespace canbus_isotp {

static const char *const TAG = "canbus_isotp";

void CanbusISOTPComponent::setup() {
    isotp_init_link(&link_, tx_can_id_, tx_buffer_, sizeof(tx_buffer_), rx_buffer_, sizeof(rx_buffer_));
    link_.user_send_can_arg = canbus_;
    #ifdef ISO_TP_RECEIVE_COMPLETE_CALLBACK
    isotp_set_rx_done_cb(&link_, [](void* link, const uint8_t* data, uint32_t size, void* user_arg){
        ((CanbusISOTPComponent*)user_arg)->on_message(data, size);
    }, this);
    #endif

    canbus_->add_callback([this](uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data){
        if (!rtr && can_id == rx_can_id_ && !extended_id) {
            isotp_on_can_message(&link_, data.data(), data.size());
        }
    });
}

void CanbusISOTPComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "canbus_isotp:");
    ESP_LOGCONFIG(TAG, "  TX std CAN ID: 0x%03" PRIx32, this->tx_can_id_);
    ESP_LOGCONFIG(TAG, "  RX std CAN ID: 0x%03" PRIx32, this->rx_can_id_);
}

void CanbusISOTPComponent::loop() {
    isotp_poll(&link_);
    #ifndef ISO_TP_RECEIVE_COMPLETE_CALLBACK
    uint32_t out_size;
    uint8_t payload[sizeof(rx_buffer_)];
    const auto& ret = isotp_receive(&link_, payload, sizeof(payload), &out_size);
    if (ISOTP_RET_OK == ret) {
        on_message(payload, out_size);
    }
    #endif
}

bool CanbusISOTPComponent::send_data(const std::vector<uint8_t> &data) {
    ESP_LOGD(TAG, "send standard id=0x%03" PRIx32 " size=%d", tx_can_id_, data.size());
    const auto& ret = isotp_send(&link_, data.data(), data.size());
    return ISOTP_RET_OK == ret;
}

void CanbusISOTPComponent::on_message(const uint8_t* data, uint32_t size) {
    ESP_LOGD(TAG, "received can iso-tp message std can_id=0x%03" PRIx32 " size=%d", rx_can_id_, size);
    std::vector<uint8_t> data_vector;
    data_vector.assign(data, data + size);
    callback_manager_(rx_can_id_, data_vector);
}

}
}
