#include "chirp_device.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace chirp {

static const char *const TAG = "chirp.device";

ChirpDevice::ChirpDevice(i2c::I2CBus *i2c_bus, uint8_t address)
    : i2c_bus_(i2c_bus), address_(address), label_("") {}

bool ChirpDevice::setup() {
  ESP_LOGD(TAG, "Setting up Chirp device at address 0x%02X", this->address_);
  
  // Try to read capacitance register to verify device exists
  auto cap = this->read_capacitance();
  if (!cap.has_value()) {
    ESP_LOGW(TAG, "Failed to communicate with device at 0x%02X", this->address_);
    return false;
  }
  
  ESP_LOGI(TAG, "Chirp device found at address 0x%02X, capacitance: %d", this->address_, cap.value());
  return true;
}

void ChirpDevice::reset() {
  ESP_LOGD(TAG, "Resetting Chirp device at 0x%02X", this->address_);
  uint8_t reset_cmd = CHIRP_REG_RESET;
  auto err = this->i2c_bus_->write(this->address_, &reset_cmd, 1);
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Failed to reset device at 0x%02X", this->address_);
  }
}

optional<uint16_t> ChirpDevice::read_capacitance() {
  return this->read_register_16bit_(CHIRP_REG_CAPACITANCE);
}

optional<uint16_t> ChirpDevice::read_temperature() {
  return this->read_register_16bit_(CHIRP_REG_TEMPERATURE);
}

optional<uint16_t> ChirpDevice::read_light() {
  return this->read_register_16bit_(CHIRP_REG_LIGHT);
}

bool ChirpDevice::request_light_measurement() {
  return this->write_register_(CHIRP_REG_MEASURE_LIGHT, CHIRP_REG_MEASURE_LIGHT);
}

bool ChirpDevice::set_i2c_address(uint8_t new_address) {
  ESP_LOGI(TAG, "Changing I2C address from 0x%02X to 0x%02X", this->address_, new_address);
  
  // Validate new address
  if (new_address < 0x01 || new_address > 0x7F) {
    ESP_LOGE(TAG, "Invalid I2C address: 0x%02X", new_address);
    return false;
  }
  
  // Write new address to register 1
  if (!this->write_register_(CHIRP_REG_ADDRESS, new_address)) {
    ESP_LOGE(TAG, "Failed to write new address to device");
    return false;
  }
  
  delay(50);  // Give device time to reconfigure
  
  // Update our internal address
  this->address_ = new_address;
  
  ESP_LOGI(TAG, "Successfully changed address to 0x%02X", new_address);
  return true;
}

std::string ChirpDevice::get_display_name() const {
  if (this->has_label()) {
    return this->label_;
  }
  char buf[8];
  snprintf(buf, sizeof(buf), "0x%02X", this->address_);
  return std::string(buf);
}

optional<uint16_t> ChirpDevice::read_register_16bit_(uint8_t reg) {
  // Write register address
  auto err = this->i2c_bus_->write(this->address_, &reg, 1);
  if (err != i2c::ERROR_OK) {
    ESP_LOGV(TAG, "Failed to write register address 0x%02X to device at 0x%02X", reg, this->address_);
    return {};
  }
  
  delay(20);  // Wait for device to prepare data
  
  // Read 2 bytes
  uint8_t data[2];
  err = this->i2c_bus_->read(this->address_, data, 2);
  if (err != i2c::ERROR_OK) {
    ESP_LOGV(TAG, "Failed to read from register 0x%02X at device 0x%02X", reg, this->address_);
    return {};
  }
  
  // Combine bytes (big endian)
  uint16_t value = (data[0] << 8) | data[1];
  return value;
}

bool ChirpDevice::write_register_(uint8_t reg, uint8_t value) {
  uint8_t data[2] = {reg, value};
  auto err = this->i2c_bus_->write(this->address_, data, 2);
  if (err != i2c::ERROR_OK) {
    ESP_LOGV(TAG, "Failed to write 0x%02X to register 0x%02X at device 0x%02X", value, reg, this->address_);
    return false;
  }
  return true;
}

}  // namespace chirp
}  // namespace esphome

