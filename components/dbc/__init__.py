import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_FILE
from esphome.components import canbus
import cantools

DBC_CONFIG = {}

CONF_CANBUS_ID = "canbus_id"
CONF_SIGNAL = "signal"
CONF_SIGNAL_CAN_ID = "can_id"
CONF_SIGNAL_START_BIT = "start_bit"
CONF_SIGNAL_LENGTH_BITS = "length_bits"
CONF_SIGNAL_ENDIANNESS = "endianness"
CONF_SIGNAL_SIGNED = "signed"
CONF_SIGNAL_DBC_ID = "dbc_id"
CONF_SIGNAL_MESSAGE = "message"
CONF_SIGNAL_SIGNAL = "signal"

MULTI_CONF = True
CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_ID): cv.string,
        cv.Required(CONF_FILE): cv.string,
    }
)

dbc_ns = cg.esphome_ns.namespace("dbc")
DBCBaseSensor = dbc_ns.class_("DBCBaseSensor", cg.Component)


def base_sensor_schema(signal_extra_manual_options = None):
    return cv.Schema({
                    cv.GenerateID(CONF_CANBUS_ID): cv.use_id(canbus.CanbusComponent),
                    cv.Required(CONF_SIGNAL): cv.Any(cv.Schema({
                        cv.Required(CONF_SIGNAL_CAN_ID): cv.int_range(0, 2**32),
                        cv.Required(CONF_SIGNAL_START_BIT): cv.int_range(0, 64 * 8),
                        cv.Required(CONF_SIGNAL_LENGTH_BITS): cv.int_range(1, 64),
                        cv.Optional(CONF_SIGNAL_ENDIANNESS, "little"): cv.one_of("little", "big", lower=True),
                        cv.Optional(CONF_SIGNAL_SIGNED, False): cv.boolean,
                        **(signal_extra_manual_options or {})
                    }), cv.Schema({
                        cv.Optional(CONF_SIGNAL_DBC_ID): cv.string,
                        cv.Required(CONF_SIGNAL_MESSAGE): cv.string,
                        cv.Required(CONF_SIGNAL_SIGNAL): cv.string,
                    })),
                }
            )


async def register_base_sensor(var, config):
    return_data = {}
    await cg.register_component(var, config)

    canbus_component = await cg.get_variable(config[CONF_CANBUS_ID])

    cg.add(var.set_canbus(canbus_component))

    config_signal = config[CONF_SIGNAL]
    if CONF_SIGNAL_CAN_ID in config_signal:
        cg.add(var.set_can_id(config_signal[CONF_SIGNAL_CAN_ID]))
        cg.add(var.set_signal(config_signal[CONF_SIGNAL_START_BIT],
            config_signal[CONF_SIGNAL_LENGTH_BITS],
            config_signal[CONF_SIGNAL_ENDIANNESS] == "little",
            config_signal[CONF_SIGNAL_SIGNED],
        ))
    else:
        dbc_id = config_signal.get(CONF_SIGNAL_DBC_ID, "default")
        dbc_db = DBC_CONFIG[dbc_id]['db']
        message_name = config_signal[CONF_SIGNAL_MESSAGE]
        signal_name = config_signal[CONF_SIGNAL_SIGNAL]

        message = next(m for m in dbc_db.messages if m.name == message_name)
        return_data['message'] = message
        signal = next(s for s in message.signals if s.name == signal_name)
        return_data['signal'] = signal

        cg.add(var.set_can_id(message.frame_id))
        cg.add(var.set_signal(signal.start,
            signal.length,
            signal.byte_order == "little_endian",
            signal.is_signed,
            signal.multiplexer_ids,
        ))
        if signal.multiplexer_ids:
            multiplexer_signal = next(s for s in message.signals if s.is_multiplexer)
            cg.add(var.set_multiplexer_signal(multiplexer_signal.start,
                multiplexer_signal.length,
                multiplexer_signal.byte_order == "little_endian",
                multiplexer_signal.is_signed,
            ))
    
    return return_data


async def to_code(config):
    conf_id = config.get(CONF_ID, "default")
    db = cantools.database.load_file(config[CONF_FILE])
    DBC_CONFIG[conf_id] = {
        'db': db
    }
