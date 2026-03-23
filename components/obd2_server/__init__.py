import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import canbus_isotp
from esphome.components import binary_sensor
from esphome.components import switch
from esphome.types import ConfigType
from .const import (
    CONF_ID,
    CONF_CANBUS_ISOTP_IDS,
    CONF_ENGINE_TYPE,
    CONF_VIN,
    CONF_SENSORS,
    CONF_SENSORS_MIL_STATUS,
    CONF_EMISSON_TESTS,
)

CODEOWNERS = ["@robin-thoni"]
DEPENDENCIES = ["canbus_isotp"]

obd2_ns = cg.esphome_ns.namespace('obd2')
OBD2ServerComponent = obd2_ns.class_('OBD2ServerComponent', cg.Component)

SENSORS_DEF = {
    CONF_SENSORS_MIL_STATUS: binary_sensor.BinarySensor,
}

MULTI_CONF = True
CONFIG_SCHEMA = cv.Schema({
        cv.GenerateID(): cv.declare_id(OBD2ServerComponent),
        cv.Required(CONF_CANBUS_ISOTP_IDS): cv.ensure_list(cv.use_id(canbus_isotp.CanbusISOTPComponent)),
        cv.Optional(CONF_ENGINE_TYPE, "spark"): cv.enum({"spark": False, "compression": True}),
        cv.Optional(CONF_VIN): cv.string,
        cv.Optional(CONF_SENSORS): cv.Schema({
            cv.Optional(sensor_name): cv.use_id(sensor_type)
                for sensor_name, sensor_type in SENSORS_DEF.items()
        }),
        cv.Optional(CONF_EMISSON_TESTS): {
            cv.int_range(0, 10): cv.use_id(binary_sensor.BinarySensor),
        },
    }).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    for canbus_isotp_config in config[CONF_CANBUS_ISOTP_IDS]:
        canbus_isotp_component = await cg.get_variable(canbus_isotp_config)
        cg.add(var.add_canbus_isotp(canbus_isotp_component))

    cg.add(var.set_engine_type(config[CONF_ENGINE_TYPE]))
    if CONF_VIN in config:
        cg.add(var.set_vin(config[CONF_VIN]))

    if CONF_SENSORS in config:
        for sensor_name, sensor_type in SENSORS_DEF.items():
            sensor = await cg.get_variable(config[CONF_SENSORS][sensor_name])
            cg.add(getattr(var, "set_" + sensor_name)(sensor))

    if CONF_EMISSON_TESTS in config:
        for sensor_idx, sensor_id in config[CONF_EMISSON_TESTS].items():
            sensor = await cg.get_variable(sensor_id)
            cg.add(var.set_emission_test(sensor_idx, sensor))


    await cg.register_component(var, config)
