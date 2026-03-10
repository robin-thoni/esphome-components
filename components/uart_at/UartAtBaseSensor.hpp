#pragma once

namespace esphome {
namespace uart_at {

const char* const TAG = "UartAtBaseSensor";

template<class T>
void UartAtBaseSensor<T>::setup() {
    if (register_urc_) {
        parent_->add_on_urc_callback([this](const AtCallbackArgs& args) {
            if (lambda_) { // TODO to be de-duplicated
                UartAtBaseSensorLambdaArgs lambdaArgs;
                auto state = lambda_(args, lambdaArgs);
                if (state.has_value()) {
                    ESP_LOGVV(TAG, "State: %s", std::to_string(*state).c_str());
                    publish_state_(*state);
                }
                if (lambdaArgs.status.has_value()) {
                    return *lambdaArgs.status;
                }
            }
    
            if (args.status == URC) {
                return CONTINUE;
            }
            return DONE;
        });
    }
}

template<class T>
void UartAtBaseSensor<T>::update() {
    auto callback = [this](const AtCallbackArgs& args) {
        if (lambda_) {
            UartAtBaseSensorLambdaArgs lambdaArgs;
            auto state = lambda_(args, lambdaArgs);
            if (state.has_value()) {
                ESP_LOGVV(TAG, "State: %s", std::to_string(*state).c_str());
                publish_state_(*state);
            }
            if (lambdaArgs.status.has_value()) {
                return *lambdaArgs.status;
            }
        }

        if (args.status == URC) {
            return CONTINUE;
        }
        return DONE;
    };

    if (query_type_ == "ex_read") {
        parent_->command_ex_read(query_name_, callback);
    } else if (query_type_ == "ex_execute") {
        parent_->command_ex_execute(query_name_, callback);
    }  else if (query_type_ == "ex_test") {
        parent_->command_ex_test(query_name_, callback);
    }
}

template<class T>
void UartAtBaseSensor<T>::set_lambda(std::function<optional<T>(const AtCallbackArgs& args, UartAtBaseSensorLambdaArgs& lambdaArgs)> lambda) {
    lambda_ = std::move(lambda);
}

template<class T>
void UartAtBaseSensor<T>::set_query_name(const std::string& query_name) {
    query_name_ = query_name;
}

template<class T>
void UartAtBaseSensor<T>::set_query_type(const std::string& query_type) {
    query_type_ = query_type;
}

template<class T>
void UartAtBaseSensor<T>::set_register_urc(bool register_urc) {
    register_urc_ = register_urc;
}

}
}
