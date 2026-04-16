import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import canbus_isotp
from esphome.components import binary_sensor
from esphome.components import sensor
from esphome.components import switch
from esphome.components import obd2
from esphome.types import ConfigType
from .const import (
    CONF_ID,
    CONF_NAME,
    CONF_SENSOR,
    CONF_TRIGGER_ID,
    CONF_CANBUS_ISOTP_IDS,
    CONF_ENGINE_TYPE,
    CONF_VIN,
    CONF_SENSORS,
    CONF_SENSORS_MIL_STATUS,
    CONF_SENSORS_ENGINE_SPEED,
    CONF_SENSORS_VEHICLE_SPEED,
    CONF_EMISSON_TESTS,
    CONF_DTC,
    CONF_DTC_TYPE,
    CONF_DTC_TYPE_PERMANENT,
    CONF_DTC_TYPE_STORED,
    CONF_ON_DTC_CLEAR,
)

CODEOWNERS = ["@robin-thoni"]
AUTO_LOAD = ["obd2"]
DEPENDENCIES = ["canbus_isotp"]

obd2_ns = cg.esphome_ns.namespace('obd2')
OBD2ServerComponent = obd2_ns.class_('OBD2ServerComponent', cg.Component)

DTCClearTrigger = obd2_ns.class_(
    "DTCClearTrigger", automation.Trigger.template()
)

SENSORS_DEF = {
    CONF_SENSORS_MIL_STATUS: binary_sensor.BinarySensor,
    CONF_SENSORS_ENGINE_SPEED: sensor.Sensor,
    CONF_SENSORS_VEHICLE_SPEED: sensor.Sensor,
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
        cv.Optional(CONF_DTC): cv.ensure_list(cv.typed_schema({
                CONF_DTC_TYPE_STORED: cv.Schema({
                    cv.Required(CONF_NAME): obd2.validate_dtc_name_,
                    cv.Required(CONF_SENSOR): cv.use_id(sensor.Sensor),
                }),
                CONF_DTC_TYPE_PERMANENT: cv.Schema({
                    cv.Required(CONF_NAME): obd2.validate_dtc_name_,
                    cv.Required(CONF_SENSOR): cv.use_id(binary_sensor.BinarySensor),
                }),
            },
            key=CONF_DTC_TYPE,
        )),
        cv.Optional(CONF_ON_DTC_CLEAR): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(DTCClearTrigger),
            }
        ),
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
            if sensor_name in config[CONF_SENSORS]:
                sensor = await cg.get_variable(config[CONF_SENSORS][sensor_name])
                cg.add(getattr(var, "set_" + sensor_name)(sensor))

    if CONF_EMISSON_TESTS in config:
        for sensor_idx, sensor_id in config[CONF_EMISSON_TESTS].items():
            sensor = await cg.get_variable(sensor_id)
            cg.add(var.set_emission_test(sensor_idx, sensor))

    if CONF_DTC in config:
        for dtc_config in config[CONF_DTC]:
            sensor = await cg.get_variable(dtc_config[CONF_SENSOR])
            cg.add(var.add_dtc(obd2.dtc_name_to_int(dtc_config[CONF_NAME]), sensor))

    for conf in config.get(CONF_ON_DTC_CLEAR, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)


    await cg.register_component(var, config)
