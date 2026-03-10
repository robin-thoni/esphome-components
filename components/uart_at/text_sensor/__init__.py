import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    CONF_LAMBDA,
)
from esphome.components.uart_at import(
    uart_at_ns,
    UartAtComponent,
    AtCallbackArgs,
    CONF_UART_AT_ID,
    SENSOR_BASE_SCHEMA,
    sensor_to_code,
)

UartAtTextSensor = uart_at_ns.class_("UartAtTextSensor", cg.PollingComponent, text_sensor.TextSensor)

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(UartAtTextSensor)
    .extend(
        {
        }
    )
    .extend(SENSOR_BASE_SCHEMA)
    .extend(cv.polling_component_schema("60s"))
)

async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)

    await sensor_to_code(var, config, cg.std_string)
