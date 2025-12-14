"""ESPHome custom component for Chirp I2C soil moisture sensors."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor"]

chirp_ns = cg.esphome_ns.namespace("chirp")
ChirpComponent = chirp_ns.class_("ChirpComponent", cg.Component)

CONF_I2C_ID = "i2c_id"
CONF_SCAN_INTERVAL = "scan_interval"
CONF_ADDRESS_RANGE = "address_range"
CONF_START = "start"
CONF_END = "end"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ChirpComponent),
            cv.GenerateID(CONF_I2C_ID): cv.use_id(i2c.I2CBus),
            cv.Optional(CONF_SCAN_INTERVAL, default="60s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_ADDRESS_RANGE): cv.Schema(
                {
                    cv.Required(CONF_START): cv.i2c_address,
                    cv.Required(CONF_END): cv.i2c_address,
                }
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    """Generate C++ code for the component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Set I2C bus parent
    i2c_bus = await cg.get_variable(config[CONF_I2C_ID])
    cg.add(var.set_i2c_bus(i2c_bus))

    # Set scan interval
    cg.add(var.set_scan_interval(config[CONF_SCAN_INTERVAL]))

    # Set address range if specified
    if CONF_ADDRESS_RANGE in config:
        range_config = config[CONF_ADDRESS_RANGE]
        cg.add(var.set_address_range(range_config[CONF_START], range_config[CONF_END]))

