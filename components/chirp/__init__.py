"""ESPHome custom component for Chirp I2C soil moisture sensors."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c

CODEOWNERS = ["@yourusername"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor"]

chirp_ns = cg.esphome_ns.namespace("chirp")

# Empty schema - the sensor platform is defined in sensor.py
CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    """No-op - component is only used as a namespace for the sensor platform."""
    pass

