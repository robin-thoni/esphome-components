#pragma once

#include "canbus_isotp_component.h"

namespace esphome {
namespace canbus_isotp {

template<typename... Ts> class SendMessage : public Action<Ts...> {
    public:
        explicit SendMessage(CanbusISOTPComponent *isotp) : isotp_(isotp) {}

        void play(Ts... x) override {
            const auto& data = data_func_ ? data_func_(x...) : data_;
            this->isotp_->send_data(data);
        }

        void set_data(const std::vector<uint8_t>& data) {
            data_ = data;
        }

        void set_data_lambda(std::function<std::vector<uint8_t>(Ts...)> func) {
            data_func_ = std::move(func);
        }

    protected:
        CanbusISOTPComponent *isotp_;

        std::vector<uint8_t> data_;
        std::function<std::vector<uint8_t>(Ts...)> data_func_;

};

class MessageTrigger : public Trigger<const std::vector<uint8_t>&> {
    public:
        MessageTrigger(CanbusISOTPComponent *isotp) {
            isotp->add_callback([this](const std::vector<uint8_t>& data) {
                this->trigger(data);
            });
        }
};

}
}
