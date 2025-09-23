#pragma once

#include "esphome/core/component.h"
#include "esphome/components/canbus/canbus.h"

namespace esphome {
namespace dbc {

struct Signal {
    uint16_t start_bit{0};
    uint16_t length_bits{1};
    bool is_little_endian{true};
    bool is_signed{false};
    std::vector<uint8_t> multiplexer_ids;
};

class DBCBaseSensor : public Component {
    public:
        void setup() override;
        void dump_config() override;

        void set_canbus(canbus::Canbus* canbus) {
            m_canbus = canbus;
        }

        void set_can_id(uint32_t can_id) {
            m_can_id = can_id;
        }

        void set_signal(uint16_t start_bit, uint16_t length_bits, bool little_endian, bool signed_, const std::vector<uint8_t>& multiplexer_ids = {}) {
            m_signal.start_bit = start_bit;
            m_signal.length_bits = length_bits;
            m_signal.is_little_endian = little_endian;
            m_signal.is_signed = signed_;
            m_signal.multiplexer_ids = multiplexer_ids;
        }

        void set_multiplexer_signal(uint16_t start_bit, uint16_t length_bits, bool little_endian, bool signed_) {
            m_is_muliplexed = true;
            m_multiplexer_signal.start_bit = start_bit;
            m_multiplexer_signal.length_bits = length_bits;
            m_multiplexer_signal.is_little_endian = little_endian;
            m_multiplexer_signal.is_signed = signed_;
        }

    protected:
        canbus::Canbus* m_canbus;
        uint32_t m_can_id{0};
        Signal m_signal;
        bool m_is_muliplexed{false};
        Signal m_multiplexer_signal;

        virtual void on_frame(uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data, float signal_value) = 0;

};

}
}