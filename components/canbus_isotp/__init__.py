import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import canbus
from .const import (
    CONF_ID,
    CONF_ON_MESSAGE,
    CONF_TRIGGER_ID,
    CONF_DATA,
    CONF_CANBUS_ID,
    CONF_TX_CAN_ID,
    CONF_RX_CAN_ID,
)

CODEOWNERS = ["@robin-thoni"]
DEPENDENCIES = ["canbus"]

canbus_isotp_ns = cg.esphome_ns.namespace('canbus_isotp')
CanbusISOTPComponent = canbus_isotp_ns.class_('CanbusISOTPComponent', cg.Component)

MessageTrigger = canbus_isotp_ns.class_(
    "MessageTrigger", automation.Trigger.template(cg.uint32, cg.std_vector.template(cg.uint8))
)

SendMessage = canbus_isotp_ns.class_("SendMessage", automation.Action)

MULTI_CONF = True
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(CanbusISOTPComponent),
    cv.GenerateID(CONF_CANBUS_ID): cv.use_id(canbus.CanbusComponent),
    cv.Required(CONF_TX_CAN_ID): cv.int_range(min=0, max=0x1FFFFFFF),
    cv.Required(CONF_RX_CAN_ID): cv.int_range(min=0, max=0x1FFFFFFF),
    cv.Optional(CONF_ON_MESSAGE): automation.validate_automation(
        {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(MessageTrigger),
        }
    ),
}).extend(cv.COMPONENT_SCHEMA)

def validate_raw_data(value):
    if isinstance(value, str):
        return value.encode("utf-8")
    if isinstance(value, list):
        return cv.Schema([cv.hex_uint8_t])(value)
    raise cv.Invalid(
        "data must either be a string wrapped in quotes or a list of bytes"
    )


SEND_MESSAGE_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_ID): cv.use_id(CanbusISOTPComponent),
    cv.Required(CONF_DATA): cv.templatable(validate_raw_data),
})

@automation.register_action("canbus_isotp.send", SendMessage, SEND_MESSAGE_SCHEMA, synchronous=True)
async def send_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)

    value = config[CONF_DATA]
    if cg.is_template(value):
        templ = await cg.templatable(value, args, cg.std_vector.template(cg.uint8))
        cg.add(var.set_data_lambda(templ))
    else:
        cg.add(var.set_data(value))

    return var



async def to_code(config):
    cg.add_build_flag("-DISO_TP_USER_SEND_CAN_ARG")
    cg.add_build_flag("-DISO_TP_RECEIVE_COMPLETE_CALLBACK")
    cg.add_build_flag("-DISO_TP_FRAME_PADDING")
    cg.add_library("isotp-c", "1.6.0", "https://github.com/SimonCahill/isotp-c#v1.6.0")

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    canbus_component = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_canbus(canbus_component))
    cg.add(var.set_tx_can_id(config[CONF_TX_CAN_ID]))
    cg.add(var.set_rx_can_id(config[CONF_RX_CAN_ID]))

    for conf in config.get(CONF_ON_MESSAGE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.uint32, "can_id"), (cg.std_vector.template(cg.uint8).operator("ref").operator("const"), 'data')], conf)

