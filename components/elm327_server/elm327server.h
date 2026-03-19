#pragma once

#include <string>

namespace elm327_server {

class ELM327Server {
public:
    ELM327Server();
    virtual ~ELM327Server() = default;

    void on_client_data(const uint8_t* data, size_t length);

    void on_can_data(uint32_t can_id, const uint8_t* data, size_t length);
    void on_isotp_data(uint32_t can_id, const uint8_t* data, size_t length);

    void poll();

protected:
    void reset();

    bool send_client_response_ok();
    bool send_client_response_unknown();
    bool send_client_response_no_data();
    bool send_client_response_raw(const std::string& data);
    bool send_client_data(const std::string& data);
    virtual bool send_client_data(const uint8_t* data, size_t length) = 0;

    virtual bool send_isotp_data(uint32_t can_id, const uint8_t* data, size_t length) = 0;

    virtual uint32_t get_time_us() const = 0;

    void log_debug(const char* message, ...) const;
    virtual void log_debug_(const char* message, va_list args) const = 0;

    void process_line(const std::string& line);

    std::string buffer_rx_;

    bool echo_enabled_;
    bool spaces_enabled_;
    bool headers_enabled_;
    std::string linefeed_;
    std::string last_command_;

    uint32_t can_query_start_ts;
    bool can_query_got_data_;

};
}
