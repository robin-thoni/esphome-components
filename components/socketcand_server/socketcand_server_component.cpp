#include "socketcand_server_component.h"
#include "esphome/components/network/util.h"
#include "esphome/core/hal.h"
#ifdef USE_LOGGER
#include "esphome/components/logger/logger.h"
#endif

namespace esphome {
namespace socketcand {

static const char *const TAG = "socketcand_server";

void SocketcandServerComponent::setup() {
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

    canbus_->add_callback([this](uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data) {
        if (!rtr && !extended_id) {
            on_can_data(can_id, data.data(), data.size());
        }
    });
}

void SocketcandServerComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "socketcand_server:");
    ESP_LOGCONFIG(TAG, "  Address: %s:%u", network::get_use_address(), this->port_);
}

void SocketcandServerComponent::loop() {
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

        on_client_data(nullptr, 1);
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
            on_client_data(nullptr, 0);
            ESP_LOGD(TAG, "Client disconnected");
            cleanup_connection_();
        }
    }

    poll();
}

void SocketcandServerComponent::set_port(uint16_t port) {
    port_ = port;
}

void SocketcandServerComponent::set_canbus(canbus::Canbus* canbus) {
    canbus_ = canbus;
}

void SocketcandServerComponent::log_socket_error_(const LogString *msg) {
    ESP_LOGW(TAG, "Socket %s: errno %d", LOG_STR_ARG(msg), errno);
}

void SocketcandServerComponent::cleanup_connection_() {
    if (this->client_ != nullptr) {
        this->client_->close();
        this->client_ = nullptr;
    }
}

bool SocketcandServerComponent::send_client_data(const uint8_t* data, size_t length) {
    if (this->client_ != nullptr) {
        auto ret = this->client_->write(data, length);
        ESP_LOGV(TAG, "Wrote %d bytes out of %d bytes", ret, length);
        return ret > 0;
    }
    return false;
}

bool SocketcandServerComponent::send_can_data(uint32_t can_id, const uint8_t* data, size_t length) {
    const auto& ret = this->canbus_->send_data(can_id, false, std::vector<uint8_t>(data, data + length));
    return canbus::ERROR_OK == ret;
}

uint32_t SocketcandServerComponent::get_time_us() const {
    return esphome::micros();
}

void SocketcandServerComponent::log_debug_(const char* message, va_list args) const {
    #ifdef USE_LOGGER
    auto *log = esphome::logger::global_logger;
    if (log == nullptr) {
        return;
    }
    log->log_vprintf_(ESPHOME_LOG_LEVEL_DEBUG, "socketcand_server.socketcandserver", 0, message, args);
    #endif
}

}
}
