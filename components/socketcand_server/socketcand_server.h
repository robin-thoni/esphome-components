#pragma once

#include <string>

namespace socketcand_server {

enum Mode {
    NO_BUS = 0,
    BCM = 1,
    RAW = 2,
    CONTROL = 3,
    ISO_TP = 4,
};

class SocketcandServer {
public:
    SocketcandServer();
    virtual ~SocketcandServer() = default;

    void on_client_data(const uint8_t* data, size_t length);

    void on_can_data(uint32_t can_id, const uint8_t* data, size_t length);

    void poll();

protected:
    bool send_client_response_error_unknown_cmd();
    bool send_client_response_error(const std::string& error);
    bool send_client_response_ok();
    bool send_client_response_raw(const std::string& data);
    bool send_client_data(const std::string& data);
    virtual bool send_client_data(const uint8_t* data, size_t length) = 0;

    virtual bool send_can_data(uint32_t can_id, const uint8_t* data, size_t length) = 0;

    virtual uint32_t get_time_us() const = 0;

    void log_debug(const char* message, ...) const;
    virtual void log_debug_(const char* message, va_list args) const = 0;

    void process_line(const std::string& line);

    std::string client_buffer_rx_;

    Mode mode_{NO_BUS};

};
}
