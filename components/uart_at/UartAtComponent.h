#pragma once

#include <queue>
#include <string>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace uart_at {

struct AtCommandArgs {
    uint32_t timeout{1000};
    std::vector<std::string> result_codes{"OK", "ERROR"};
};

enum AtCommandStatus {
    OK = 1,
    TIMEOUT = 2,
    URC = 3,
};

struct AtCallbackArgs {
    AtCommandStatus status;
    std::string response;

    void dump() const;
};

enum AtCallbackStatus {
    DONE = 1,
    CONTINUE = 2,
    CONTINUE_RESET = 3,
};

typedef std::function<AtCallbackStatus(const AtCallbackArgs& args)> AtCallback;

struct AtQuery {
    AtCommandArgs args;
    std::string data;
    uint32_t sent_timestamp{0};
    AtCallback callback;
    AtCallbackArgs callback_args;
};

class UartAtComponent : public Component, public uart::UARTDevice {
public:
    void setup() override;
    void dump_config() override;
    void loop() override;

    bool command(std::string cmd, AtCallback callback, const AtCommandArgs& args = {});
    bool command_basic(std::string x, std::string n, AtCallback callback, const AtCommandArgs& args = {});
    bool command_s_parameter(std::string n, std::string m, AtCallback callback, const AtCommandArgs& args = {});
    bool command_ex_test(std::string x, AtCallback callback, const AtCommandArgs& args = {});
    bool command_ex_read(std::string x, AtCallback callback, const AtCommandArgs& args = {});
    bool command_ex_write(std::string x, std::string data, AtCallback callback, const AtCommandArgs& args = {});
    bool command_ex_execute(std::string x, AtCallback callback, const AtCommandArgs& args = {});

    void set_newline(const std::string& nl);

    void set_echo_enabled(bool echo_enabled);

    void add_on_urc_callback(std::function<void(const AtCallbackArgs&)>&& cb);

    Trigger<const AtCallbackArgs&>* get_on_urc_trigger() {
        return &this->on_urc_trigger_;
    }

protected:
    bool enqueue_query(const AtQuery& query);

    void handle_line(const std::string& line);

    AtCallbackStatus current_query_callback(AtCallbackArgs* args = nullptr);

    AtQuery* m_current_query{nullptr};
    std::queue<AtQuery> m_send_queue;

    // bool m_is_echo_enabled{false}; // TODO?

    std::string m_nl{"\r\n"};

    std::string m_buffer;

    CallbackManager<void(const AtCallbackArgs&)> on_urc_callbacks_;
    Trigger<const AtCallbackArgs&>  on_urc_trigger_;

};
}
}
