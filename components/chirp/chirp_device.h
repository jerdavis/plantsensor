#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include <string>

namespace esphome {
namespace chirp {

// Chirp I2C Registers
static const uint8_t CHIRP_REG_CAPACITANCE = 0x00;  // Moisture sensor capacitance
static const uint8_t CHIRP_REG_ADDRESS = 0x01;       // Change I2C address
static const uint8_t CHIRP_REG_MEASURE_LIGHT = 0x03; // Request light measurement
static const uint8_t CHIRP_REG_LIGHT = 0x04;         // Read light value
static const uint8_t CHIRP_REG_TEMPERATURE = 0x05;   // Temperature sensor
static const uint8_t CHIRP_REG_RESET = 0x06;         // Reset sensor

/**
 * Represents a single Chirp soil moisture sensor device.
 * Handles I2C communication and sensor value reading for one device.
 */
class ChirpDevice {
 public:
  ChirpDevice(i2c::I2CBus *i2c_bus, uint8_t address);

  /**
   * Initialize the device and verify it's a Chirp sensor.
   * Returns true if device responds correctly.
   */
  bool setup();

  /**
   * Reset the Chirp sensor.
   */
  void reset();

  /**
   * Read the capacitance value (moisture).
   * Returns raw capacitance reading (0-65535).
   */
  optional<uint16_t> read_capacitance();

  /**
   * Read the temperature value.
   * Returns temperature in 0.1°C units (divide by 10 for actual temperature).
   */
  optional<uint16_t> read_temperature();

  /**
   * Read the light level.
   * Must call request_light_measurement() first and wait ~1 second.
   * Returns light level in lux.
   */
  optional<uint16_t> read_light();

  /**
   * Request a light measurement.
   * Must wait ~1000ms before reading with read_light().
   */
  bool request_light_measurement();

  /**
   * Change the I2C address of this device.
   * WARNING: This permanently changes the sensor's address until changed again.
   */
  bool set_i2c_address(uint8_t new_address);

  /**
   * Get the current I2C address.
   */
  uint8_t get_address() const { return this->address_; }

  /**
   * Update the address after a successful address change.
   */
  void update_address(uint8_t new_address) { this->address_ = new_address; }

  /**
   * Set the friendly label for this device.
   */
  void set_label(const std::string &label) { this->label_ = label; }

  /**
   * Get the friendly label for this device.
   */
  const std::string &get_label() const { return this->label_; }

  /**
   * Check if device has a custom label.
   */
  bool has_label() const { return !this->label_.empty(); }

  /**
   * Get a display name for the device (label if set, otherwise address).
   */
  std::string get_display_name() const;

 protected:
  i2c::I2CBus *i2c_bus_;
  uint8_t address_;
  std::string label_;

  /**
   * Read a 16-bit value from a register.
   */
  optional<uint16_t> read_register_16bit_(uint8_t reg);

  /**
   * Write a single byte to a register.
   */
  bool write_register_(uint8_t reg, uint8_t value);
};

}  // namespace chirp
}  // namespace esphome

