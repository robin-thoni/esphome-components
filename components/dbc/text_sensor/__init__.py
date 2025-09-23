import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_UNIT_OF_MEASUREMENT
from esphome.components import text_sensor
from esphome.components import dbc
import cantools

DEPENDENCIES = ["canbus", "dbc"]

DBCTextSensor = dbc.dbc_ns.class_("DBCTextSensor", dbc.DBCBaseSensor)

CONF_CHOICES = "choices"

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(DBCTextSensor)
    .extend(dbc.base_sensor_schema(signal_extra_manual_options=
        {
            cv.Required(CONF_CHOICES): cv.string,
        }))
    .extend(
    )
)

async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    sensor_data = await dbc.register_base_sensor(var, config)
    signal = sensor_data.get('signal')
    if signal and signal.choices and CONF_CHOICES not in config:
        choices = signal.choices
    elif CONF_CHOICES in config[dbc.CONF_SIGNAL]:
        choices = config[dbc.CONF_SIGNAL][CONF_CHOICES]
    else:
        choices = None
    
    if choices:
        for k, v in choices.items():
            if isinstance(v, cantools.database.namedsignalvalue.NamedSignalValue):
                v = v.name
            cg.add(var.add_choice(k, v))
