#include "chirp_sensor.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace chirp {

static const char *const TAG = "chirp.sensor";

void ChirpSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Chirp sensor '%s' at address 0x%02X...", 
                this->device_name_.c_str(), this->address_);
  
  // Verify device responds by trying to read capacitance
  auto test_read = this->read_register_16bit_(CHIRP_REG_CAPACITANCE);
  if (!test_read.has_value()) {
    ESP_LOGE(TAG, "Failed to communicate with Chirp sensor '%s' at address 0x%02X", 
             this->device_name_.c_str(), this->address_);
    this->mark_failed();
    return;
  }
  
  ESP_LOGI(TAG, "Chirp sensor '%s' found at 0x%02X, capacitance: %d", 
           this->device_name_.c_str(), this->address_, test_read.value());
  
  // Reset device
  this->write_register_(CHIRP_REG_RESET, CHIRP_REG_RESET);
  delayMicroseconds(50000);  // 50ms delay
  
  ESP_LOGCONFIG(TAG, "Chirp sensor '%s' setup complete", this->device_name_.c_str());
}

void ChirpSensor::update() {
  uint32_t now = millis();
  
  // Read moisture
  if (this->moisture_sensor_ != nullptr) {
    auto capacitance = this->read_register_16bit_(CHIRP_REG_CAPACITANCE);
    if (capacitance.has_value()) {
      float moisture = this->capacitance_to_moisture_(capacitance.value());
      this->moisture_sensor_->publish_state(moisture);
      ESP_LOGD(TAG, "'%s': Capacitance=%d, Moisture=%.0f%%", 
               this->device_name_.c_str(), capacitance.value(), moisture);
    } else {
      ESP_LOGW(TAG, "'%s': Failed to read capacitance", this->device_name_.c_str());
    }
  }
  
  // Read temperature
  if (this->temperature_sensor_ != nullptr) {
    auto temp_raw = this->read_register_16bit_(CHIRP_REG_TEMPERATURE);
    if (temp_raw.has_value()) {
      float temperature = this->raw_to_temperature_(temp_raw.value());
      this->temperature_sensor_->publish_state(temperature);
      ESP_LOGD(TAG, "'%s': Temperature=%.1f°C", this->device_name_.c_str(), temperature);
    } else {
      ESP_LOGW(TAG, "'%s': Failed to read temperature", this->device_name_.c_str());
    }
  }
  
  // Handle light measurement (requires request and delay)
  if (this->light_sensor_ != nullptr) {
    if (!this->light_requested_) {
      // Request light measurement
      if (this->write_register_(CHIRP_REG_MEASURE_LIGHT, CHIRP_REG_MEASURE_LIGHT)) {
        this->light_requested_ = true;
        this->last_light_request_ = now;
        ESP_LOGV(TAG, "'%s': Requested light measurement", this->device_name_.c_str());
      }
    } else if (now - this->last_light_request_ >= LIGHT_MEASUREMENT_DELAY) {
      // Read light measurement
      auto light = this->read_register_16bit_(CHIRP_REG_LIGHT);
      if (light.has_value()) {
        this->light_sensor_->publish_state(light.value());
        ESP_LOGD(TAG, "'%s': Light=%d lx", this->device_name_.c_str(), light.value());
      } else {
        ESP_LOGW(TAG, "'%s': Failed to read light", this->device_name_.c_str());
      }
      this->light_requested_ = false;
    }
  }
}

void ChirpSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Chirp Sensor '%s':", this->device_name_.c_str());
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Moisture", this->moisture_sensor_);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Light", this->light_sensor_);
}

optional<uint16_t> ChirpSensor::read_register_16bit_(uint8_t reg) {
  // Write register address
  if (this->write(&reg, 1) != i2c::ERROR_OK) {
    ESP_LOGV(TAG, "Failed to write register address 0x%02X to device at 0x%02X", reg, this->address_);
    return {};
  }
  
  delayMicroseconds(5000);  // Wait 5ms for device to prepare data
  
  // Read 2 bytes
  uint8_t data[2];
  if (this->read(data, 2) != i2c::ERROR_OK) {
    ESP_LOGV(TAG, "Failed to read from register 0x%02X at device 0x%02X", reg, this->address_);
    return {};
  }
  
  // Combine bytes (big endian)
  uint16_t value = (data[0] << 8) | data[1];
  return value;
}

bool ChirpSensor::write_register_(uint8_t reg, uint8_t value) {
  uint8_t data[2] = {reg, value};
  if (this->write(data, 2) != i2c::ERROR_OK) {
    ESP_LOGV(TAG, "Failed to write 0x%02X to register 0x%02X at device 0x%02X", value, reg, this->address_);
    return false;
  }
  return true;
}

float ChirpSensor::capacitance_to_moisture_(uint16_t capacitance) {
  // Use calibration from original config
  float moisture = (static_cast<float>(capacitance) - MOISTURE_DRY) / (MOISTURE_WET - MOISTURE_DRY) * 100.0f;
  
  // Clamp to 0-100%
  if (moisture < 0.0f) moisture = 0.0f;
  if (moisture > 100.0f) moisture = 100.0f;
  
  return moisture;
}

float ChirpSensor::raw_to_temperature_(uint16_t raw_temp) {
  // Temperature is in 0.1°C units
  return static_cast<float>(raw_temp) / 10.0f;
}

}  // namespace chirp
}  // namespace esphome

