#include "elm327_server_component.h"
#include "esphome/components/network/util.h"
#include "esphome/core/hal.h"
#ifdef USE_LOGGER
#include "esphome/components/logger/logger.h"
#endif

namespace esphome {
namespace elm327 {

static const char *const TAG = "elm327_server";

void ELM327ServerComponent::setup() {
    this->server_ = socket::socket_ip_loop_monitored(SOCK_STREAM, 0);  // monitored for incoming connections
    if (this->server_ == nullptr) {
        this->log_socket_error_(LOG_STR("creation"));
        this->mark_failed();
        return;
    }
    int enable = 1;
    int err = this->server_->setsockopt(SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (err != 0) {
        this->log_socket_error_(LOG_STR("reuseaddr"));
        // we can still continue
    }
    err = this->server_->setblocking(false);
    if (err != 0) {
        this->log_socket_error_(LOG_STR("non-blocking"));
        this->mark_failed();
        return;
    }

    struct sockaddr_storage server;

    socklen_t sl = socket::set_sockaddr_any((struct sockaddr *) &server, sizeof(server), this->port_);
    if (sl == 0) {
        this->log_socket_error_(LOG_STR("set sockaddr"));
        this->mark_failed();
        return;
    }

    err = this->server_->bind((struct sockaddr *) &server, sizeof(server));
    if (err != 0) {
        this->log_socket_error_(LOG_STR("bind"));
        this->mark_failed();
        return;
    }

    err = this->server_->listen(1);  // Only one client at a time
    if (err != 0) {
        this->log_socket_error_(LOG_STR("listen"));
        this->mark_failed();
        return;
    }


    isotp_init_link(&link_, 0x7DF, tx_buffer_, sizeof(tx_buffer_), rx_buffer_, sizeof(rx_buffer_));
    link_.user_send_can_arg = canbus_;

    canbus_->add_callback([this](uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data) {
        // WARNING: This currently assumes no parallel streams from different ECUs
        if (!rtr && can_id >= 0x7E8 && can_id <= 0x7EF && !extended_id) {
            on_can_data(can_id, data.data(), data.size());
            isotp_on_can_message(&link_, data.data(), data.size());
            uint32_t out_size;
            uint8_t payload[sizeof(rx_buffer_)];
            const auto& ret = isotp_receive(&link_, payload, sizeof(payload), &out_size);
            if (ISOTP_RET_OK == ret) {
                on_isotp_data(can_id, payload, out_size);
            }
        }
    });
}

void ELM327ServerComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "elm327_server:");
    ESP_LOGCONFIG(TAG, "  Address: %s:%u", network::get_use_address(), this->port_);
}

void ELM327ServerComponent::loop() {
    if (this->client_ == nullptr && this->server_ != nullptr && this->server_->ready()) {
        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int enable = 1;

        this->client_ = this->server_->accept_loop_monitored((struct sockaddr *) &source_addr, &addr_len);
        if (this->client_ == nullptr) {
            return;
        }
        char peername[socket::SOCKADDR_STR_LEN];
        this->client_->getpeername_to(peername);
        ESP_LOGD(TAG, "New client: %s", peername);

        int err = this->client_->setsockopt(IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));
        if (err != 0) {
            this->log_socket_error_(LOG_STR("nodelay"));
            this->cleanup_connection_();
            return;
        }
        err = this->client_->setblocking(false);
        if (err != 0) {
            this->log_socket_error_(LOG_STR("non-blocking"));
            this->cleanup_connection_();
            return;
        }
    }
    if (this->client_ != nullptr) {
        uint8_t buf[1025];
        ssize_t read = this->client_->read(buf, sizeof(buf) - 1);
        if (read > 0) {
            buf[read] = '\0';
            ESP_LOGV(TAG, "Got data: %s", buf);
            on_client_data(buf, read);
        }
        else if (read == 0) {
            ESP_LOGD(TAG, "Client disconnected");
            cleanup_connection_();
        }
    }

    isotp_poll(&link_);

    poll();
}

void ELM327ServerComponent::set_port(uint16_t port) {
    port_ = port;
}

void ELM327ServerComponent::set_canbus(canbus::Canbus* canbus) {
    canbus_ = canbus;
}

void ELM327ServerComponent::log_socket_error_(const LogString *msg) {
    ESP_LOGW(TAG, "Socket %s: errno %d", LOG_STR_ARG(msg), errno);
}

void ELM327ServerComponent::cleanup_connection_() {
    if (this->client_ != nullptr) {
        this->client_->close();
        this->client_ = nullptr;
    }
}

bool ELM327ServerComponent::send_client_data(const uint8_t* data, size_t length) {
    if (this->client_ != nullptr) {
        auto ret = this->client_->write(data, length);
        ESP_LOGV(TAG, "Wrote %d bytes out of %d bytes", ret, length);
        return ret > 0;
    }
    return false;
}

bool ELM327ServerComponent::send_isotp_data(uint32_t can_id, const uint8_t* data, size_t length) {
    const auto& ret = isotp_send_with_id(&link_, can_id, data, length);
    return ISOTP_RET_OK == ret;
}

uint32_t ELM327ServerComponent::get_time_us() const {
    return esphome::micros();
}

void ELM327ServerComponent::log_debug_(const char* message, va_list args) const {
    #ifdef USE_LOGGER
    auto *log = esphome::logger::global_logger;
    if (log == nullptr) {
        return;
    }
    log->log_vprintf_(ESPHOME_LOG_LEVEL_DEBUG, "elm327_server.elm327server", 0, message, args);
    #endif
}

}
}
