#include "obd2.h"

#include <esphome/core/helpers.h>

namespace esphome {
namespace obd2 {

static const char* dtc_categories = "PCBU";

std::string dtc_to_string(uint16_t dtc) {
    return std::string(1, dtc_categories[dtc >> 14]) +
        format_hex_char((dtc >> 12) & 0x03) +
        format_hex_char((dtc >> 8) & 0x0F) +
        format_hex_char((dtc >> 4) & 0x0F) +
        format_hex_char(dtc & 0x0F);
}

}
}
