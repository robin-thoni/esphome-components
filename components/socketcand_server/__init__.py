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
DEPENDENCIES = ["network", "canbus"]
AUTO_LOAD = ["socket"]

socketcand_ns = cg.esphome_ns.namespace("esphome").namespace('socketcand')
SocketcandServerComponent = socketcand_ns.class_('SocketcandServerComponent', cg.Component)


def _consume_sockets(config: ConfigType) -> ConfigType:
    from esphome.components import socket

    # Allow only a single client/session + listening
    socket.consume_sockets(1 + 1, "socketcand")(config)
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(SocketcandServerComponent),
        cv.Optional(CONF_PORT, 29536): cv.port,
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
