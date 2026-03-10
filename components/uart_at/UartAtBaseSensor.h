#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/uart_at/UartAtComponent.h"

namespace esphome {
namespace uart_at {

    struct UartAtBaseSensorLambdaArgs {
        optional<AtCallbackStatus> status;
    };

    template<class T>
    class UartAtBaseSensor : public PollingComponent
                                        , public Parented<UartAtComponent> {
    public:
        virtual void setup() override;
        virtual void update() override;

        void set_lambda(std::function<optional<T>(const AtCallbackArgs& args, UartAtBaseSensorLambdaArgs& lambdaArgs)> lambda);

        void set_query_name(const std::string& query_name);

        void set_query_type(const std::string& query_type);

        void set_register_urc(bool register_urc);

    protected:
        virtual void publish_state_(const T&) = 0;

        std::function<optional<T>(const AtCallbackArgs& args, UartAtBaseSensorLambdaArgs& lambdaArgs)> lambda_;

        std::string query_name_;

        std::string query_type_;
        
        bool register_urc_;
    };
}
}

#include "UartAtBaseSensor.hpp"
