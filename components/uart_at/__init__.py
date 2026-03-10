import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components.uart import UARTComponent, UARTDevice, register_uart_device
from esphome.const import (
    CONF_ID,
    CONF_LAMBDA,
    CONF_TRIGGER_ID,
    CONF_UART_ID,
    CONF_TIMEOUT,
)
from .const import (
    CONF_UART_AT_ID,
    CONF_QUERY_NAME,
    CONF_QUERY_TYPE,
    CONF_QUERY_DATA,
    CONF_REGISTER_URC,

    CONF_NEWLINE,
    # CONF_ECHO_ENABLED, # TODO?
    CONF_RESULT_CODES,
    CONF_ON_URC,

    CONF_ON_RESPONSE,
)

uart_at_ns = cg.esphome_ns.namespace("uart_at")
UartAtComponent = uart_at_ns.class_("UartAtComponent", cg.Component, UARTDevice)
AtCallbackArgs = uart_at_ns.class_("AtCallbackArgs")
UartAtBaseSensorLambdaArgs = uart_at_ns.class_("UartAtBaseSensorLambdaArgs")

UrcTrigger = uart_at_ns.class_(
    "UrcTrigger", automation.Trigger.template(AtCallbackArgs)
)

MULTI_CONF = True
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(UartAtComponent),
            cv.GenerateID(CONF_UART_ID): cv.use_id(UARTComponent),
            cv.Optional(CONF_NEWLINE): cv.string,
            # cv.Optional(CONF_ECHO_ENABLED): cv.boolean, // TODO?
            cv.Optional(CONF_ON_URC): automation.validate_automation(single=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

SENSOR_BASE_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_UART_AT_ID): cv.use_id(UartAtComponent),
            cv.Optional(CONF_QUERY_NAME): cv.string,
            cv.Optional(CONF_QUERY_DATA): cv.string,
            cv.Optional(CONF_QUERY_TYPE): cv.string,
            cv.Optional(CONF_REGISTER_URC): cv.boolean,
            cv.Optional(CONF_LAMBDA): cv.returning_lambda,
        }
    )
)

async def sensor_to_code(var, config, type):
    parent = await cg.get_variable(config[CONF_UART_AT_ID])
    cg.add(var.set_parent(parent))
    if CONF_QUERY_NAME in config:
        cg.add(var.set_query_name(config[CONF_QUERY_NAME]))
    if CONF_QUERY_DATA in config:
        cg.add(var.set_query_data(config[CONF_QUERY_DATA]))
    if CONF_QUERY_TYPE in config:
        cg.add(var.set_query_type(config[CONF_QUERY_TYPE]))
    if CONF_REGISTER_URC in config:
        cg.add(var.set_register_urc(config[CONF_REGISTER_URC]))


    if CONF_LAMBDA in config:
        template_ = await cg.process_lambda(
            config[CONF_LAMBDA],
            [(AtCallbackArgs.operator("ref").operator("const"), 'args'), (UartAtBaseSensorLambdaArgs.operator("ref"), "lambdaArgs")],
            return_type=cg.optional.template(type),
        )
        cg.add(var.set_lambda(template_))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await register_uart_device(var, config)

    if CONF_NEWLINE in config:
        cg.add(var.set_newline(config[CONF_NEWLINE]))

    # if CONF_ECHO_ENABLED in config: # TODO?
    #     cg.add(var.set_echo_enabled(config[CONF_ECHO_ENABLED]))

    if CONF_ON_URC in config:
        await automation.build_automation(var.get_on_urc_trigger(), [(AtCallbackArgs.operator("ref").operator("const"), 'args')], config[CONF_ON_URC])


CommandAction = uart_at_ns.class_("CommandAction", automation.Action)

COMMAND_BASIC_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_ID): cv.use_id(UartAtComponent),
            cv.Required(CONF_QUERY_NAME): cv.templatable(cv.string),
            cv.Optional(CONF_QUERY_DATA): cv.templatable(cv.string),
            cv.Optional(CONF_TIMEOUT): cv.positive_time_period,
            cv.Optional(CONF_RESULT_CODES): cv.ensure_list(cv.string),
            cv.Optional(CONF_ON_RESPONSE): automation.validate_automation(single=True),
        }
    )
)

async def command_to_code_(config, action_id, template_arg, args, query_type):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)

    if CONF_QUERY_NAME in config:
        cg.add(var.set_query_name(await cg.templatable(config[CONF_QUERY_NAME], args, cg.std_string)))
    if CONF_QUERY_DATA in config:
        cg.add(var.set_query_data(await cg.templatable(config[CONF_QUERY_DATA], args, cg.std_string)))
    cg.add(var.set_query_type(query_type))

    if CONF_TIMEOUT in config:
        cg.add(var.set_timeout(int(await cg.templatable(config[CONF_TIMEOUT].total_milliseconds, args, cg.std_string))))
    if CONF_RESULT_CODES in config:
        cg.add(var.set_result_codes(config[CONF_RESULT_CODES]))

    if CONF_ON_RESPONSE in config:
        await automation.build_automation(var.get_on_response_trigger(), [(AtCallbackArgs.operator("ref").operator("const"), 'args'), (UartAtBaseSensorLambdaArgs.operator("ref"), "lambdaArgs")], config[CONF_ON_RESPONSE])

    return var

@automation.register_action("uart_at.command_basic", CommandAction, COMMAND_BASIC_SCHEMA)
async def command_to_code(config, action_id, template_arg, args):
    return await command_to_code_(config, action_id, template_arg, args, "basic")

@automation.register_action("uart_at.s_parameter", CommandAction, COMMAND_BASIC_SCHEMA)
async def command_to_code(config, action_id, template_arg, args):
    return await command_to_code_(config, action_id, template_arg, args, "s_parameter")

@automation.register_action("uart_at.ex_read", CommandAction, COMMAND_BASIC_SCHEMA)
async def command_to_code(config, action_id, template_arg, args):
    return await command_to_code_(config, action_id, template_arg, args, "ex_read")

@automation.register_action("uart_at.ex_execute", CommandAction, COMMAND_BASIC_SCHEMA)
async def command_to_code(config, action_id, template_arg, args):
    return await command_to_code_(config, action_id, template_arg, args, "ex_execute")

@automation.register_action("uart_at.ex_test", CommandAction, COMMAND_BASIC_SCHEMA)
async def command_to_code(config, action_id, template_arg, args):
    return await command_to_code_(config, action_id, template_arg, args, "ex_test")

@automation.register_action("uart_at.ex_write", CommandAction, COMMAND_BASIC_SCHEMA)
async def command_to_code(config, action_id, template_arg, args):
    return await command_to_code_(config, action_id, template_arg, args, "ex_write")
