import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import canbus_isotp

from esphome.types import ConfigType
from .const import (
    CONF_ID,
    CONF_NAME,
    CONF_TRIGGER_ID,
    CONF_CANBUS_ISOTP_BROADCAST_ID,
    CONF_CANBUS_ISOTP_ECUS_IDS,
)

CODEOWNERS = ["@robin-thoni"]
AUTO_LOAD = ["obd2"]
DEPENDENCIES = ["canbus_isotp"]

obd2_ns = cg.esphome_ns.namespace('obd2')
OBD2ClientComponent = obd2_ns.class_('OBD2ClientComponent', cg.Component)

MULTI_CONF = True
CONFIG_SCHEMA = cv.Schema({
        cv.GenerateID(): cv.declare_id(OBD2ClientComponent),
        cv.Required(CONF_CANBUS_ISOTP_BROADCAST_ID): cv.use_id(canbus_isotp.CanbusISOTPComponent),
        cv.Required(CONF_CANBUS_ISOTP_ECUS_IDS): cv.ensure_list(cv.use_id(canbus_isotp.CanbusISOTPComponent)),
    }).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    canbus_isotp_component = await cg.get_variable(config[CONF_CANBUS_ISOTP_BROADCAST_ID])
    cg.add(var.set_canbus_isotp_broadcast(canbus_isotp_component))
    for canbus_isotp_config in config[CONF_CANBUS_ISOTP_ECUS_IDS]:
        canbus_isotp_component = await cg.get_variable(canbus_isotp_config)
        cg.add(var.add_canbus_isotp_ecu(canbus_isotp_component))

    await cg.register_component(var, config)

