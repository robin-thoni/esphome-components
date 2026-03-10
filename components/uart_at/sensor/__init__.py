import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
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

UartAtSensor = uart_at_ns.class_("UartAtSensor", cg.PollingComponent, sensor.Sensor)

CONFIG_SCHEMA = (
    sensor.sensor_schema(UartAtSensor)
    .extend(
        {
        }
    )
    .extend(SENSOR_BASE_SCHEMA)
    .extend(cv.polling_component_schema("60s"))
)

async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    
    await sensor_to_code(var, config, float)
