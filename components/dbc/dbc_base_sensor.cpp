#include "dbc_base_sensor.h"

namespace esphome {
namespace dbc {

static const char *const TAG = "dbc";

float parse_signal(
    const std::vector<uint8_t>& data,
    uint16_t startBit,
    uint16_t length,
    bool isLittleEndian,
    bool isSigned
) {
    if (startBit + length > data.size() * 8) {
        return NAN;
    }
    // TODO better error handling
    /*if (data.size() != 8) {
        throw std::invalid_argument("CAN frame must be 8 bytes");
    }
    if (length == 0 || length > 64) {
        throw std::invalid_argument("Invalid signal length");
    }*/

    uint64_t raw = 0;

    if (isLittleEndian) {
        // Intel: bit numbering increases with byte index
        for (uint16_t i = 0; i < length; i++) {
            uint16_t bitIndex = startBit + i;
            uint16_t byteIndex = bitIndex / 8;
            uint16_t bitPos = bitIndex % 8;

            if (data[byteIndex] & (1u << bitPos)) {
                raw |= (1ULL << i);
            }
        }
    } else {
        // Motorola (big endian): bits are counted differently
        for (uint16_t i = 0; i < length; i++) {
            uint16_t bitIndex = startBit - i;
            uint16_t byteIndex = bitIndex / 8;
            uint16_t bitPos = 7 - (bitIndex % 8);

            if (data[byteIndex] & (1u << bitPos)) {
                raw |= (1ULL << (length - 1 - i));
            }
        }
    }

    if (isSigned) {
        // Sign extend
        uint64_t signBit = 1ULL << (length - 1);
        if (raw & signBit) {
            int64_t signedVal = static_cast<int64_t>(raw | (~((1ULL << length) - 1)));
            return static_cast<float>(signedVal);
        }
    }

    return static_cast<float>(raw);
}

void DBCBaseSensor::setup() {
    m_canbus->add_callback([this](uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data){
        if (can_id == m_can_id) {
            bool is_good_frame = true;
            if (m_is_muliplexed) {
                auto multiplexer_signal_value = (uint8_t)parse_signal(data, m_multiplexer_signal.start_bit, m_multiplexer_signal.length_bits, m_multiplexer_signal.is_little_endian, m_multiplexer_signal.is_signed);
                if (std::find(m_signal.multiplexer_ids.begin(), m_signal.multiplexer_ids.end(), multiplexer_signal_value) == m_signal.multiplexer_ids.end()) {
                    is_good_frame = false;
                }
            }
            if (is_good_frame) {
                auto signal_value = parse_signal(data, m_signal.start_bit, m_signal.length_bits, m_signal.is_little_endian, m_signal.is_signed);
                on_frame(can_id, extended_id, rtr, data, signal_value);
            }
        }
    });
}

void DBCBaseSensor::dump_config() {
    ESP_LOGCONFIG(TAG, "dbc:");
    ESP_LOGCONFIG(TAG, "  CAN ID: 0x%08" PRIx32, this->m_can_id);
    ESP_LOGCONFIG(TAG, "  Start Bit: " PRId16, this->m_signal.start_bit);
    ESP_LOGCONFIG(TAG, "  Length Bits: " PRId16, this->m_signal.length_bits);
    ESP_LOGCONFIG(TAG, "  Endianness: " , this->m_signal.is_little_endian ? "Little" : "Big");
    ESP_LOGCONFIG(TAG, "  Signed: " , this->m_signal.is_signed ? "Yes" : "No");
}

}
}
