import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.components import obd2_client
from esphome.components.obd2_client.const import (
    CONF_SENSOR_OBD2_CLIENT_ID,
    CONF_SENSOR_SERVICE,
    CONF_TYPE,
)

DEPENDENCIES = ["obd2_client"]

OBD2ClientDTCTextSensor = obd2_client.obd2_ns.class_(
    "OBD2ClientDTCTextSensor",
    text_sensor.TextSensor,
    cg.PollingComponent,
    cg.Parented.template(obd2_client.OBD2ClientComponent),
)

CONFIG_SCHEMA = cv.typed_schema(
    {
        "dtc": text_sensor.text_sensor_schema()
        .extend(cv.polling_component_schema("60s"))
        .extend(
            {
                cv.GenerateID(): cv.declare_id(OBD2ClientDTCTextSensor),
                cv.GenerateID(CONF_SENSOR_OBD2_CLIENT_ID): cv.use_id(obd2_client.OBD2ClientComponent),
                cv.Required(CONF_SENSOR_SERVICE): cv.int_,
            }
        ),
    }
)

async def to_code(config):

    if config[CONF_TYPE]== "dtc":
        parent = await cg.get_variable(config[CONF_SENSOR_OBD2_CLIENT_ID])
        var = await text_sensor.new_text_sensor(config)
        await cg.register_parented(var, parent)
        await cg.register_component(var, config)

        cg.add(var.set_service(config[CONF_SENSOR_SERVICE]))
