#pragma once

#include "UartAtComponent.h"

namespace esphome {
namespace uart_at {

template<typename... Ts> class CommandAction : public Action<Ts...> {
public:
    explicit CommandAction(UartAtComponent *parent) : parent_(parent) {}

    void play(Ts... x) override {
        auto callback = [this](const AtCallbackArgs& args) {
            UartAtBaseSensorLambdaArgs lambdaArgs;
            on_response_trigger_.trigger(args, lambdaArgs);
            if (lambdaArgs.status.has_value()) {
                return *lambdaArgs.status;
            }
    
            if (args.status == URC) {
                return CONTINUE;
            }
            return DONE;
        };

        AtCommandArgs args;

        auto timeout = timeout_.value(x...);
        if (timeout) {
            args.timeout = timeout;
        }
        if (result_codes_.has_value()) {
            args.result_codes = *result_codes_;
        }

        auto query_name = query_name_.value(x...);
        auto query_data = query_data_.value(x...);


        if (query_type_ == "ex_read") {
            parent_->command_ex_read(query_name, callback, args);
        } else if (query_type_ == "ex_execute") {
            parent_->command_ex_execute(query_name, callback, args);
        } else if (query_type_ == "ex_test") {
            parent_->command_ex_test(query_name, callback, args);
        } else if (query_type_ == "ex_write") {
            parent_->command_ex_write(query_name, query_data, callback, args);
        } else if (query_type_ == "basic") {
            parent_->command_basic(query_name, query_data, callback, args);
        } else if (query_type_ == "s_parameter") {
            parent_->command_s_parameter(query_name, query_data, callback, args);
        }
    }

    void set_query_type(const std::string& query_type) {
        query_type_ = query_type;
    }

    void set_result_codes(const std::vector<std::string>& result_codes) {
        result_codes_ = result_codes;
    }

    Trigger<const AtCallbackArgs&, UartAtBaseSensorLambdaArgs&>* get_on_response_trigger() {
        return &this->on_response_trigger_;
    }

protected:
    UartAtComponent* parent_;

    TEMPLATABLE_VALUE(std::string, query_name)
    TEMPLATABLE_VALUE(std::string, query_data)
    std::string query_type_;

    TEMPLATABLE_VALUE(uint32_t, timeout)
    optional<std::vector<std::string>> result_codes_;

    Trigger<const AtCallbackArgs&, UartAtBaseSensorLambdaArgs&> on_response_trigger_;
};

}
}
