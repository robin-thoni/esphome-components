import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_UNIT_OF_MEASUREMENT
from esphome.components import sensor
from esphome.components import dbc

DEPENDENCIES = ["canbus", "dbc"]

DBCSensor = dbc.dbc_ns.class_("DBCSensor", dbc.DBCBaseSensor)


CONFIG_SCHEMA = (
    sensor.sensor_schema(DBCSensor)
    .extend(dbc.base_sensor_schema())
    .extend(
        {
        }
    )
)

async def to_code(config):
    var = await sensor.new_sensor(config)
    sensor_data = await dbc.register_base_sensor(var, config)
    signal = sensor_data.get('signal')
    if signal and signal.unit and CONF_UNIT_OF_MEASUREMENT not in config:
        cg.add(var.set_unit_of_measurement(signal.unit))
