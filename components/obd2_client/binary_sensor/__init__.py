import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.components import obd2_client
from esphome.components.obd2_client.const import (
    CONF_LAMBDA,
    CONF_SENSOR_OBD2_CLIENT_ID,
    CONF_SENSOR_SERVICE,
    CONF_SENSOR_PID,
    CONF_TYPE,
)

DEPENDENCIES = ["obd2_client"]

OBD2ClientPIDBinarySensor = obd2_client.obd2_ns.class_(
    "OBD2ClientPIDBinarySensor",
    binary_sensor.BinarySensor,
    cg.PollingComponent,
    cg.Parented.template(obd2_client.OBD2ClientComponent),
)

CONFIG_SCHEMA = cv.typed_schema(
    {
        "pid": binary_sensor.binary_sensor_schema()
        .extend(cv.polling_component_schema("60s"))
        .extend(
            {
                cv.GenerateID(): cv.declare_id(OBD2ClientPIDBinarySensor),
                cv.GenerateID(CONF_SENSOR_OBD2_CLIENT_ID): cv.use_id(obd2_client.OBD2ClientComponent),
                cv.Required(CONF_SENSOR_SERVICE): cv.int_,
                cv.Required(CONF_SENSOR_PID): cv.int_,
                cv.Optional(CONF_LAMBDA): cv.returning_lambda,
            }
        ),
    }
)

async def to_code(config):

    if config[CONF_TYPE]== "pid":
        parent = await cg.get_variable(config[CONF_SENSOR_OBD2_CLIENT_ID])
        var = await binary_sensor.new_binary_sensor(config)
        await cg.register_parented(var, parent)
        await cg.register_component(var, config)

        cg.add(var.set_service(config[CONF_SENSOR_SERVICE]))
        cg.add(var.set_pid(config[CONF_SENSOR_PID]))
        if CONF_LAMBDA in config:
            lambda_ = await cg.process_lambda(
                config[CONF_LAMBDA], [(cg.std_vector.template(cg.uint8), "x")], return_type=cg.optional.template(cg.float_)
            )
            cg.add(var.set_lambda(lambda_))
