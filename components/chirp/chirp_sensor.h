#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace chirp {

// Chirp I2C Registers
static const uint8_t CHIRP_REG_CAPACITANCE = 0x00;  // Moisture sensor capacitance
static const uint8_t CHIRP_REG_TEMPERATURE = 0x05;   // Temperature sensor
static const uint8_t CHIRP_REG_MEASURE_LIGHT = 0x03; // Request light measurement
static const uint8_t CHIRP_REG_LIGHT = 0x04;         // Read light value
static const uint8_t CHIRP_REG_RESET = 0x06;         // Reset sensor

/**
 * Simple sensor component for a single Chirp I2C soil moisture sensor.
 * Creates 3 sensors: moisture, temperature, and light.
 */
class ChirpSensor : public PollingComponent, public i2c::I2CDevice {
 public:
  ChirpSensor() = default;

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_moisture_sensor(sensor::Sensor *sensor) { this->moisture_sensor_ = sensor; }
  void set_temperature_sensor(sensor::Sensor *sensor) { this->temperature_sensor_ = sensor; }
  void set_light_sensor(sensor::Sensor *sensor) { this->light_sensor_ = sensor; }
  void set_device_name(const std::string &name) { this->device_name_ = name; }

 protected:
  sensor::Sensor *moisture_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *light_sensor_{nullptr};
  std::string device_name_;
  
  uint32_t last_light_request_{0};
  bool light_requested_{false};
  
  // Calibration values
  static constexpr float MOISTURE_DRY = 263.0f;
  static constexpr float MOISTURE_WET = 483.0f;
  static constexpr uint32_t LIGHT_MEASUREMENT_DELAY = 1000;  // 1 second

  // I2C communication helpers
  optional<uint16_t> read_register_16bit_(uint8_t reg);
  bool write_register_(uint8_t reg, uint8_t value);
  
  float capacitance_to_moisture_(uint16_t capacitance);
  float raw_to_temperature_(uint16_t raw_temp);
};

}  // namespace chirp
}  // namespace esphome

