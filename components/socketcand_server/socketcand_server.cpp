#include "socketcand_server.h"
#include <algorithm>
#include <cstdarg>
#include <iomanip>
#include <sstream>
#include <vector>

namespace socketcand_server {

static bool parse_hex(const std::string& hex_str, std::vector<uint8_t>& bytes) {

    std::string cleaned;
    cleaned.reserve(hex_str.size());
    for (char c : hex_str) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            cleaned += std::toupper(c);
        }
    }

    if (cleaned.size() % 2 != 0) {
        cleaned = "0" + cleaned;
    }

    bytes.reserve(bytes.size() + hex_str.size() / 2);

    for (std::size_t i = 0; i < cleaned.size(); i += 2) {
        uint8_t high = static_cast<uint8_t>(std::toupper(cleaned[i]) - (std::isalpha(cleaned[i]) ? 'A' - 10 : '0'));
        uint8_t low = static_cast<uint8_t>(std::toupper(cleaned[i+1]) - (std::isalpha(cleaned[i+1]) ? 'A' - 10 : '0'));
        bytes.push_back((high << 4) | low);
    }
    return true;
}

static bool parse_hex_int(const std::string& hex_str, uint32_t& value) {
    std::vector<uint8_t> bytes;
    if (!parse_hex(hex_str, bytes)) {
        return false;
    }
    value = 0;
    for (int i = 0; i < sizeof(value) && i < bytes.size(); ++i) {
        value = (value << 8) | bytes[i];
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

std::vector<std::string> tokenize_socketcand_cmd(const std::string& cmd) {
    std::vector<std::string> tokens;
    std::istringstream iss(cmd);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

static std::string clean_socketcand_cmd(std::string cmd) {

    cmd.erase(std::remove_if(cmd.begin(), cmd.end(),
        [](unsigned char c) {
            return c == '\t' || c == '\r' || c == '\n' || !std::isprint(c);
        }), cmd.end());

    return cmd.substr(2, cmd.length() - 4);
}

static bool starts_with(const std::string& str, const std::string& needle) {
    return str.rfind(needle, 0) == 0;
}

SocketcandServer::SocketcandServer() {
    client_buffer_rx_.reserve(1024);
}

void SocketcandServer::on_client_data(const uint8_t* data, size_t length) {
    if (data == nullptr) {
        if (length) {
            send_client_response_raw("hi");
        }
        mode_ = NO_BUS;
        return;
    }
    client_buffer_rx_ += std::string((const char*)data, length);
    // TODO make sure the string doesn't grow indefinitely when there's no '>' in it

    std::size_t start = 0;

    while(true) {
        std::size_t cr_pos = client_buffer_rx_.find('>', start);
        if (cr_pos == std::string::npos) {
            break;
        }

        std::string line = client_buffer_rx_.substr(start, cr_pos - start + 1);

        process_line(line);

        start = cr_pos + 1;
    }

    if (start > 0) {
        client_buffer_rx_.erase(0, start);
    }
}

void SocketcandServer::on_can_data(uint32_t can_id, const uint8_t* data, size_t length) {
    if (mode_ == RAW) {
        const auto& now = get_time_us(); // TODO 64 bits timestamp
        const auto& now_sec = now / 1000000.0f;
        const auto& now_str = std::to_string(now_sec);
        const auto& can_id_hex = format_hex({(uint8_t)((can_id >> 8) & 0xFF), (uint8_t)(can_id & 0xFF)}, false).substr(1);
        const auto& data_hex = format_hex(data, length, false);
        send_client_response_raw("frame " + can_id_hex + " " + now_str + " " + data_hex);
    }
}

void SocketcandServer::poll() {
}

bool SocketcandServer::send_client_response_error_unknown_cmd() {
    return send_client_response_error("unknown command");
}

bool SocketcandServer::send_client_response_error(const std::string& error) {
    return send_client_response_raw("error " + error);
}

bool SocketcandServer::send_client_response_ok() {
    return send_client_response_raw("ok");
}

bool SocketcandServer::send_client_response_raw(const std::string& data) {
    return send_client_data("< " + data + " >");
}

bool SocketcandServer::send_client_data(const std::string& data) {
    log_debug("send_client_data: %s", data.c_str());
    return send_client_data((const uint8_t*)data.c_str(), data.length());
}

void SocketcandServer::log_debug(const char* message, ...) const {
    va_list arg;
    va_start(arg, message);
    this->log_debug_(message, arg);
    va_end(arg);
}

void SocketcandServer::process_line(const std::string& line) {
    log_debug("process_line: %s", line.c_str());
    auto full_cmd = clean_socketcand_cmd(line);
    log_debug("process_line (clean): %s", full_cmd.c_str());

    const auto& tokens = tokenize_socketcand_cmd(full_cmd);
    if ( tokens.size() == 0) {
        return;
    }
    
    const auto& cmd = tokens[0];

    if (cmd == "bcmmode") {
        mode_ = BCM;
        send_client_response_ok();
    } else if (cmd == "rawmode") {
        mode_ = RAW;
        send_client_response_ok();
    } else if (cmd == "controlmode") {
        mode_ = CONTROL;
        send_client_response_ok();
    } else if (cmd == "isotpmode") {
        mode_ = ISO_TP;
        send_client_response_ok();
    } else if (cmd == "echo") {
        send_client_response_raw("echo");
    } else if (mode_ == NO_BUS) {
        if (cmd == "open") {
            if (tokens.size() == 2 && tokens[1] == "can0") {
                send_client_response_ok();
                mode_ = BCM;
            } else {
                send_client_response_error("could not open bus");
            }
        } else if (tokens.size() > 2 && (tokens[1] == "B" || tokens[1] == "C")) {
            send_client_response_error("configuring interface " + cmd);
        } else {
            send_client_response_error_unknown_cmd();
        }
    } else if (mode_ == BCM) {
        send_client_response_error("not implemented");
    } else if (mode_ == RAW) {
        if (cmd == "send") {
            if (tokens.size() >= 3) {
                uint32_t can_id;
                parse_hex_int(tokens[1], can_id);
                std::vector<uint8_t> data;
                for (int i = 3; i < tokens.size(); ++i) {
                    parse_hex(tokens[i], data);
                }
                send_can_data(can_id, data.data(), data.size());
            } else {
                log_debug("Bad 'send' command: %s", full_cmd.c_str());
            }
        } else {
            send_client_response_error_unknown_cmd();
        }
    } else if (mode_ == CONTROL) {
        send_client_response_error("not implemented");
    } else if (mode_ == ISO_TP) {
        send_client_response_error("not implemented");
    } else {
        send_client_response_error("internal state error");
    }
}

}
