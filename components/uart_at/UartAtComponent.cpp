#include "UartAtComponent.h"
#include <string.h>
#include "esphome/core/log.h"


static const char *const TAG = "uart_at";

namespace esphome {
namespace uart_at {

std::string debug_str(const std::string& data) {
    auto response = data;
    for (size_t i = 0; i < response.length(); ++i) {
        if (response[i] == '\r') {
            response.replace(i, 1, "←");
        }
        if (response[i] == '\n') {
            response.replace(i, 1, "↓");
        }
    }
    return response;
}

void AtCallbackArgs::dump() const {
    ESP_LOGI(TAG, "Status: %i, Response: %s", this->status, debug_str(response).c_str());
}

void UartAtComponent::setup() {
    on_urc_callbacks_.add([this](const AtCallbackArgs& args) {
        on_urc_trigger_.trigger(args);
    });
}

void UartAtComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "UartAt:");
    // ESP_LOGCONFIG(TAG, "  Echo Enabled: %s", m_is_echo_enabled ? "Yes" : "No"); // TODO?
    ESP_LOGCONFIG(TAG, "  Newline: %s", debug_str(m_nl).c_str());
}

void UartAtComponent::loop() {

    if (m_current_query == nullptr && m_send_queue.size() > 0) {
        m_current_query = &m_send_queue.front();
        m_current_query->sent_timestamp = millis();
        write_str(m_current_query->data.c_str());
    }

    while(available()) {
        auto byte = read();
        m_buffer += byte;
        if (m_nl.length() <= m_buffer.length() && std::equal(m_nl.rbegin(), m_nl.rend(), m_buffer.rbegin())) {
            handle_line(m_buffer.substr(0, m_buffer.length() - m_nl.length()));
            m_buffer = "";
        }
    }

    if (m_current_query != nullptr && millis() >= m_current_query->sent_timestamp + m_current_query->args.timeout) {
        m_current_query->callback_args.status = TIMEOUT;
        auto response = m_current_query->callback_args.response;
        auto status = current_query_callback();
        if (status == DONE) {
            ESP_LOGW(TAG, "Query timeout; Buffer: %s", debug_str(response).c_str());
        }
    }
}

bool UartAtComponent::command(std::string cmd, AtCallback callback, const AtCommandArgs& args) {
    AtQuery query;
    query.args = args;
    query.callback = callback;
    query.data = cmd + m_nl;

    return enqueue_query(query);
}

bool UartAtComponent::command_basic(std::string x, std::string n, AtCallback callback, const AtCommandArgs& args) {
    return command("AT" + x + n, callback, args);
}

bool UartAtComponent::command_s_parameter(std::string n, std::string m, AtCallback callback, const AtCommandArgs& args) {
    std::string cmd;
    if (m.length()) {
        cmd = "ATS" + n + "=" + m;
    } else {
        cmd = "ATS" + n;
    }

    return command(cmd, callback, args);
}

bool UartAtComponent::command_ex_test(std::string x, AtCallback callback, const AtCommandArgs& args) {
    return command("AT+" + x + "=?", callback, args);
}

bool UartAtComponent::command_ex_read(std::string x, AtCallback callback, const AtCommandArgs& args) {
    return command("AT+" + x + "?", callback, args);
}

bool UartAtComponent::command_ex_write(std::string x, std::string data, AtCallback callback, const AtCommandArgs& args) {
    return command("AT+" + x + "=" + data, callback, args);
}

bool UartAtComponent::command_ex_execute(std::string x, AtCallback callback, const AtCommandArgs& args) {
    return command("AT+" + x, callback, args);
}

void UartAtComponent::set_newline(const std::string& nl) {
    m_nl = nl;
}

// void UartAtComponent::set_echo_enabled(bool echo_enabled) { // TODO?
//     m_is_echo_enabled = echo_enabled;
// }

void UartAtComponent::add_on_urc_callback(std::function<void(const AtCallbackArgs&)>&& cb) {
    on_urc_callbacks_.add(std::move(cb));
}

bool UartAtComponent::enqueue_query(const AtQuery& query) {
    m_send_queue.push(query);
    return true;
}

void UartAtComponent::handle_line(const std::string& line) {
    ESP_LOGVV(TAG, "Handling line: %s", debug_str(line).c_str());
    if (line[0] == '+' || line[0] == '*') {
        AtCallbackArgs args;
        args.status = URC;
        args.response = line;

        if (m_current_query != nullptr) {
            current_query_callback(&args);
        }

        on_urc_callbacks_.call(args);

        ESP_LOGV(TAG, "Got URC: %s", debug_str(line).c_str());
    } else if (m_current_query != nullptr) {
        m_current_query->callback_args.response += line + m_nl;

        bool is_response_complete = false;
        for (const auto& result_code : m_current_query->args.result_codes) {
            if (line == result_code) {
                is_response_complete = true;
                break;
            }
        }

        if (is_response_complete) {
            if (m_nl.length() <= m_current_query->callback_args.response.length() && std::equal(m_nl.begin(), m_nl.end(), m_current_query->callback_args.response.begin())) {
                m_current_query->callback_args.response.erase(0, m_nl.length());
            }
            m_current_query->callback_args.response.erase(m_current_query->callback_args.response.length() - m_nl.length());
            m_current_query->callback_args.status = OK;
            current_query_callback();
        }
    } else {
        ESP_LOGW(TAG, "Discarding unknown data: %s", debug_str(m_buffer).c_str());
    }
}

AtCallbackStatus UartAtComponent::current_query_callback(AtCallbackArgs* args) {
    AtCallbackStatus status = DONE;
    if (m_current_query->callback) {
        status = m_current_query->callback(args != nullptr ? *args : m_current_query->callback_args);
    }
    if (status == DONE) {
        m_current_query = nullptr;
        m_send_queue.pop();
    } else if (status == CONTINUE_RESET) {
        m_current_query->sent_timestamp = millis();
    }
    return status;
}

}
}
