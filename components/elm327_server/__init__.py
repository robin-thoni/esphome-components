import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.core import CoroPriority, coroutine_with_priority
from esphome.components import canbus
from esphome.types import ConfigType
from .const import (
    CONF_ID,
    CONF_PORT,
    CONF_CANBUS_ID,
)

CODEOWNERS = ["@robin-thoni"]
DEPENDENCIES = ["network", "canbus", "canbus_isotp"]
AUTO_LOAD = ["socket"]

elm327_ns = cg.esphome_ns.namespace("esphome").namespace('elm327')
ELM327ServerComponent = elm327_ns.class_('ELM327ServerComponent', cg.Component)


def _consume_sockets(config: ConfigType) -> ConfigType:
    from esphome.components import socket

    # Allow only a single diagnostic client/session + listening
    socket.consume_sockets(1 + 1, "elm327_server")(config)
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(ELM327ServerComponent),
        cv.Optional(CONF_PORT, 35000): cv.port,
        cv.GenerateID(CONF_CANBUS_ID): cv.use_id(canbus.CanbusComponent),
    }).extend(cv.COMPONENT_SCHEMA),
    _consume_sockets,
)


@coroutine_with_priority(CoroPriority.NETWORK_SERVICES)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_port(config[CONF_PORT]))
    canbus_component = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_canbus(canbus_component))
    await cg.register_component(var, config)
