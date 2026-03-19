#pragma once

#include "esphome/core/component.h"
#include "esphome/components/canbus/canbus.h"
#include "esphome/components/socket/socket.h"

#include "elm327server.h"
#include "isotp.h"

namespace esphome {
namespace elm327 {

class ELM327ServerComponent : public Component, elm327_server::ELM327Server {
public:
    void setup() override;
    float get_setup_priority() const { return setup_priority::AFTER_WIFI; }
    void dump_config() override;
    void loop() override;

    void set_port(uint16_t port);

    void set_canbus(canbus::Canbus* canbus);

protected:
    void log_socket_error_(const LogString *msg);

    void cleanup_connection_();

    virtual bool send_client_data(const uint8_t* data, size_t length) override;

    virtual bool send_isotp_data(uint32_t can_id, const uint8_t* data, size_t length) override;

    virtual uint32_t get_time_us() const override;

    virtual void log_debug_(const char* message, va_list args) const override;

    uint16_t port_{35000};
    std::unique_ptr<socket::Socket> server_;
    std::unique_ptr<socket::Socket> client_;

    canbus::Canbus* canbus_;

    uint8_t tx_buffer_[256];
    uint8_t rx_buffer_[256];

    IsoTpLink link_;
};
}
}
