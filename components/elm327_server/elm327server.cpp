#include "elm327server.h"
#include <algorithm>
#include <cstdarg>
#include <iomanip>
#include <sstream>
#include <vector>

namespace elm327_server {

static bool parse_hex(const std::string& hex_str, std::vector<uint8_t>& bytes) {
    bytes.clear();
    bytes.reserve(hex_str.size() / 2);

    std::string cleaned;
    cleaned.reserve(hex_str.size());
    for (char c : hex_str) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            cleaned += std::toupper(c);
        }
    }

    if (cleaned.size() % 2 != 0) {
        return false;
    }

    for (std::size_t i = 0; i < cleaned.size(); i += 2) {
        uint8_t high = static_cast<uint8_t>(std::toupper(cleaned[i]) - (std::isalpha(cleaned[i]) ? 'A' - 10 : '0'));
        uint8_t low = static_cast<uint8_t>(std::toupper(cleaned[i+1]) - (std::isalpha(cleaned[i+1]) ? 'A' - 10 : '0'));
        bytes.push_back((high << 4) | low);
    }
    return true;
}

static std::string format_hex(const uint8_t* data, size_t length, bool spaces) {
    std::ostringstream oss;
    for (size_t i = 0; i < length; ++i) {
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << (unsigned)data[i];
        if (i + 1 < length && spaces) {
            oss << ' ';
        }
    }
    return oss.str();
}

static std::string format_hex(const std::vector<uint8_t>& data, bool spaces) {
    return format_hex(data.data(), data.size(), spaces);
}

static std::string clean_elm327_cmd(std::string cmd) {
    // Remove spaces, tabs; uppercase; strip non-printable
    cmd.erase(std::remove_if(cmd.begin(), cmd.end(),
        [](unsigned char c) {
            return c == ' ' || c == '\t' || c == '\r' || !std::isprint(c);
        }), cmd.end());
    
    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
        [](unsigned char c) { return std::toupper(c); });

    return cmd;
}

static bool starts_with(const std::string& str, const std::string& needle) {
    return str.rfind(needle, 0) == 0;
}

ELM327Server::ELM327Server() {
    buffer_rx_.reserve(1024);
    reset();
}

void ELM327Server::on_client_data(const uint8_t* data, size_t length) {
    if (echo_enabled_) {
        send_client_data(data, length);
    }
    buffer_rx_ += std::string((const char*)data, length);
    // TODO make sure the string doesn't grow indefinitely when there's no '\r' in it

    std::size_t start = 0;

    while(true) {
        std::size_t cr_pos = buffer_rx_.find('\r', start);
        if (cr_pos == std::string::npos) {
            break;
        }

        std::string line = buffer_rx_.substr(start, cr_pos - start);

        process_line(line);

        start = cr_pos + 1;
    }

    if (start > 0) {
        buffer_rx_.erase(0, start);
    }
}

void ELM327Server::on_can_data(uint32_t can_id, const uint8_t* data, size_t length) {
    if (can_query_start_ts && headers_enabled_) {
        can_query_start_ts = get_time_us();
        can_query_got_data_ = true;

        const auto& data_hex = format_hex(data, length, spaces_enabled_);
        const auto& can_id_hex = format_hex({(uint8_t)((can_id >> 8) & 0xFF), (uint8_t)(can_id & 0xFF)}, false).substr(1);
        const auto& header_hex = headers_enabled_ ? can_id_hex + (spaces_enabled_ ? " " : "") : "";
        send_client_data(header_hex + data_hex + linefeed_);
    }
}


void ELM327Server::on_isotp_data(uint32_t can_id, const uint8_t* data, size_t length) {
    if (can_query_start_ts && !headers_enabled_) {
        can_query_start_ts = get_time_us();
        can_query_got_data_ = true;

        if (length > 7) {
            const auto& length_hex = format_hex({(uint8_t)((length >> 8) & 0xFF), (uint8_t)(length & 0xFF)}, false).substr(1);
            send_client_data(length_hex + linefeed_);
            for (int i = 0, start_index = 0; start_index < length; ++i, start_index += (start_index == 0 ? 6 : 7)) {
                auto l = (start_index == 0 ? 6 : 7);
                if (start_index + l > length) {
                    l = length - start_index;
                }
                const auto& data_hex = format_hex(data + start_index, l, spaces_enabled_);
                send_client_data(std::to_string(i) + ":" + (spaces_enabled_ ? " " : "") + data_hex.substr() + linefeed_);
            }
        } else {
            const auto& data_hex = format_hex(data, length, spaces_enabled_);
            send_client_data(data_hex + linefeed_);
        }
    }
}

void ELM327Server::poll() {
    const auto& now_ts = get_time_us();
    if (can_query_start_ts && now_ts - can_query_start_ts > 300000) {
        if (can_query_got_data_) {
            send_client_data(linefeed_ + ">");
        } else {
            send_client_response_no_data();
        }
        can_query_start_ts = 0;
        can_query_got_data_ = false;
    }
}

void ELM327Server::reset() {
    echo_enabled_ = true;
    spaces_enabled_ = true;
    headers_enabled_ = false;
    linefeed_ = "\r";
    last_command_.clear();
    can_query_start_ts = 0;
    can_query_got_data_ = false;
}

bool ELM327Server::send_client_response_ok() {
    return send_client_response_raw("OK");
}

bool ELM327Server::send_client_response_unknown() {
    return send_client_response_raw("?");
}

bool ELM327Server::send_client_response_no_data() {
    return send_client_response_raw("NO DATA");
}

bool ELM327Server::send_client_response_raw(const std::string& data) {
    return send_client_data(data + linefeed_ + linefeed_ + ">");
}

bool ELM327Server::send_client_data(const std::string& data) {
    log_debug("send_client_data: %s", data.c_str());
    return send_client_data((const uint8_t*)data.c_str(), data.length());
}

void ELM327Server::log_debug(const char* message, ...) const {
    va_list arg;
    va_start(arg, message);
    this->log_debug_(message, arg);
    va_end(arg);
}

void ELM327Server::process_line(const std::string& line) {
    log_debug("process_line: %s", line.c_str());
    auto cmd = clean_elm327_cmd(line);
    log_debug("process_line (clean): %s", cmd.c_str());
    if (line == "") {
        if (last_command_ != "") {
            cmd = last_command_;
        } else {
            send_client_response_unknown();
            return;
        }
    }
    last_command_ = cmd;
    if (starts_with(cmd, "AT")) {
        if (cmd == "ATZ") {
            reset();
            send_client_response_raw(linefeed_ + linefeed_ + "ELM327 v1.5");
        } else if (cmd == "ATE0") {
            echo_enabled_ = false;
            send_client_response_ok();
        } else if (cmd == "ATE1") {
            echo_enabled_ = true;
            send_client_data("ATE1\r"); // Manually handle echo when re-enabling
            send_client_response_ok();
        } else if (cmd == "ATS0") {
            spaces_enabled_ = false;
            send_client_response_ok();
        } else if (cmd == "ATS1") {
            spaces_enabled_ = true;
            send_client_response_ok();
        } else if (cmd == "ATH0") {
            headers_enabled_ = false;
            send_client_response_ok();
        } else if (cmd == "ATH1") {
            headers_enabled_ = true;
            send_client_response_ok();
        } else if (cmd == "ATL0") {
            linefeed_ = "\r";
            send_client_response_ok();
        } else if (cmd == "ATL1") {
            linefeed_ = "\r\n";
            send_client_response_ok();
        } else {
            send_client_response_unknown();
        }
    } else {
        std::vector<uint8_t> data;
        if (parse_hex(cmd, data)) {
            if (send_isotp_data(0x7DF, data.data(), data.size())) {
                can_query_start_ts = get_time_us();
                can_query_got_data_ = false;
            } else {
                send_client_response_no_data();
            }
        } else {
            send_client_response_unknown();
        }
    }
}

}
