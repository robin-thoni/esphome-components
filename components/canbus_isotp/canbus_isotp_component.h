#pragma once

#include "esphome/core/component.h"
#include "esphome/components/canbus/canbus.h"

#include "isotp.h"

namespace esphome {
namespace canbus_isotp {

class CanbusISOTPComponent : public Component {
    public:
        void setup() override;
        void dump_config() override;
        void loop() override;

        void set_canbus(canbus::Canbus* canbus) {
            canbus_ = canbus;
        }

        void set_tx_can_id(uint32_t can_id) {
            tx_can_id_ = can_id;
        }

        void set_rx_can_id(uint32_t can_id) {
            rx_can_id_ = can_id;
        }

        void add_callback(
            std::function<void(const std::vector<uint8_t> &data)> callback) {
            this->callback_manager_.add(std::move(callback));
        }

        bool send_data(const std::vector<uint8_t> &data);

        void on_message(const uint8_t* data, uint32_t size);

    protected:
        canbus::Canbus* canbus_{nullptr};
        uint32_t tx_can_id_{0};
        uint32_t rx_can_id_{0};

        uint8_t tx_buffer_[256];
        uint8_t rx_buffer_[256];

        IsoTpLink link_;

        CallbackManager<void(const std::vector<uint8_t> &data)> callback_manager_{};

};

}
}