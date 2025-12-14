#include "chirp.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include <cstring>

namespace esphome {
namespace chirp {

static const char *const TAG = "chirp";

// Calibration values from original config
static const float MOISTURE_DRY = 263.0f;
static const float MOISTURE_WET = 483.0f;

// Light measurement timing
static const uint32_t LIGHT_MEASUREMENT_DELAY = 1000;  // 1 second

// Storage format for labels (simple key-value store)
struct LabelStorage {
  uint8_t address;
  char label[32];
};

void ChirpComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Chirp component...");
  
  // Load stored labels
  this->load_labels_();
  
  // Perform initial scan
  this->scan_for_devices_();
  
  // Reset all devices
  for (auto *device : this->devices_) {
    device->reset();
    delay(50);
  }
  
  // Register services
  register_service(&ChirpComponent::set_device_address, "set_address",
                   {"old_address", "new_address"});
  register_service(&ChirpComponent::set_device_label, "set_label",
                   {"address", "label"});
  register_service(&ChirpComponent::rescan_bus, "rescan");
  
  ESP_LOGCONFIG(TAG, "Chirp component setup complete. Found %d device(s)", this->devices_.size());
}

void ChirpComponent::loop() {
  uint32_t now = millis();
  
  // Periodic rescan for new devices
  if (now - this->last_scan_ > this->scan_interval_) {
    this->scan_for_devices_();
    this->last_scan_ = now;
  }
  
  // Update sensors every 5 seconds (similar to original config)
  if (this->update_counter_++ % 100 == 0) {  // Assuming loop runs ~50Hz
    for (auto *device : this->devices_) {
      this->update_device_(device);
    }
  }
}

void ChirpComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Chirp Component:");
  ESP_LOGCONFIG(TAG, "  Scan Interval: %u ms", this->scan_interval_);
  ESP_LOGCONFIG(TAG, "  Address Range: 0x%02X - 0x%02X", this->scan_start_, this->scan_end_);
  ESP_LOGCONFIG(TAG, "  Devices Found: %d", this->devices_.size());
  
  for (auto *device : this->devices_) {
    ESP_LOGCONFIG(TAG, "    - Address: 0x%02X, Label: %s", 
                  device->get_address(), 
                  device->has_label() ? device->get_label().c_str() : "(none)");
  }
}

void ChirpComponent::set_device_address(uint8_t old_address, uint8_t new_address) {
  ESP_LOGI(TAG, "Service call: set_device_address(0x%02X -> 0x%02X)", old_address, new_address);
  
  // Find device with old address
  ChirpDevice *device = nullptr;
  for (auto *dev : this->devices_) {
    if (dev->get_address() == old_address) {
      device = dev;
      break;
    }
  }
  
  if (!device) {
    ESP_LOGE(TAG, "Device at address 0x%02X not found", old_address);
    return;
  }
  
  // Check if new address is already in use
  for (auto *dev : this->devices_) {
    if (dev->get_address() == new_address) {
      ESP_LOGE(TAG, "Address 0x%02X is already in use", new_address);
      return;
    }
  }
  
  // Change the address
  if (device->set_i2c_address(new_address)) {
    // Update sensors with new address
    auto old_sensors = this->sensors_[old_address];
    this->sensors_.erase(old_address);
    this->sensors_[new_address] = old_sensors;
    
    // Update sensor names
    std::string device_name = device->get_display_name();
    if (old_sensors.moisture) {
      old_sensors.moisture->set_name(this->get_sensor_name_("Soil Moisture", device_name).c_str());
    }
    if (old_sensors.temperature) {
      old_sensors.temperature->set_name(this->get_sensor_name_("Soil Temperature", device_name).c_str());
    }
    if (old_sensors.light) {
      old_sensors.light->set_name(this->get_sensor_name_("Soil Light", device_name).c_str());
    }
    
    this->save_labels_();
    ESP_LOGI(TAG, "Successfully changed device address to 0x%02X", new_address);
  }
}

void ChirpComponent::set_device_label(uint8_t address, const std::string &label) {
  ESP_LOGI(TAG, "Service call: set_device_label(0x%02X, '%s')", address, label.c_str());
  
  // Find device
  ChirpDevice *device = nullptr;
  for (auto *dev : this->devices_) {
    if (dev->get_address() == address) {
      device = dev;
      break;
    }
  }
  
  if (!device) {
    ESP_LOGE(TAG, "Device at address 0x%02X not found", address);
    return;
  }
  
  // Set label
  device->set_label(label);
  
  // Update sensor names
  auto &sensors = this->sensors_[address];
  std::string device_name = device->get_display_name();
  
  if (sensors.moisture) {
    sensors.moisture->set_name(this->get_sensor_name_("Soil Moisture", device_name).c_str());
  }
  if (sensors.temperature) {
    sensors.temperature->set_name(this->get_sensor_name_("Soil Temperature", device_name).c_str());
  }
  if (sensors.light) {
    sensors.light->set_name(this->get_sensor_name_("Soil Light", device_name).c_str());
  }
  
  this->save_labels_();
  ESP_LOGI(TAG, "Successfully set label for device at 0x%02X", address);
}

void ChirpComponent::rescan_bus() {
  ESP_LOGI(TAG, "Service call: rescan_bus()");
  this->scan_for_devices_();
}

void ChirpComponent::scan_for_devices_() {
  ESP_LOGD(TAG, "Scanning I2C bus for Chirp devices (0x%02X - 0x%02X)...", 
           this->scan_start_, this->scan_end_);
  
  std::vector<uint8_t> found_addresses;
  
  // Scan address range
  for (uint8_t addr = this->scan_start_; addr <= this->scan_end_; addr++) {
    // Skip if we already have this device
    bool already_have = false;
    for (auto *dev : this->devices_) {
      if (dev->get_address() == addr) {
        already_have = true;
        found_addresses.push_back(addr);
        break;
      }
    }
    
    if (already_have) {
      continue;
    }
    
    // Try to create and setup a device at this address
    ChirpDevice *test_device = new ChirpDevice(this->parent_, addr);
    if (test_device->setup()) {
      // Valid device found
      found_addresses.push_back(addr);
      this->add_device_(addr);
      delete test_device;  // We'll create a new one in add_device_
    } else {
      delete test_device;
    }
    
    delay(10);  // Small delay between probes
  }
  
  // Remove devices that are no longer present
  for (size_t i = 0; i < this->devices_.size();) {
    uint8_t addr = this->devices_[i]->get_address();
    bool found = false;
    for (uint8_t found_addr : found_addresses) {
      if (found_addr == addr) {
        found = true;
        break;
      }
    }
    
    if (!found) {
      ESP_LOGW(TAG, "Device at 0x%02X is no longer responding, removing...", addr);
      this->remove_device_(addr);
      // Don't increment i, as we removed an element
    } else {
      i++;
    }
  }
  
  ESP_LOGD(TAG, "Scan complete. Active devices: %d", this->devices_.size());
}

void ChirpComponent::add_device_(uint8_t address) {
  ESP_LOGI(TAG, "Adding new Chirp device at address 0x%02X", address);
  
  // Create device
  ChirpDevice *device = new ChirpDevice(this->parent_, address);
  
  // Try to load label from storage
  for (auto *existing_dev : this->devices_) {
    // This is just for initial setup, labels are loaded in load_labels_
  }
  
  this->devices_.push_back(device);
  
  // Create sensors for this device
  this->create_sensors_(address, device);
}

void ChirpComponent::remove_device_(uint8_t address) {
  // Remove from devices list
  for (size_t i = 0; i < this->devices_.size(); i++) {
    if (this->devices_[i]->get_address() == address) {
      delete this->devices_[i];
      this->devices_.erase(this->devices_.begin() + i);
      break;
    }
  }
  
  // Remove sensors (they'll be marked as unavailable)
  if (this->sensors_.count(address) > 0) {
    auto &sensors = this->sensors_[address];
    if (sensors.moisture) {
      sensors.moisture->publish_state(NAN);
    }
    if (sensors.temperature) {
      sensors.temperature->publish_state(NAN);
    }
    if (sensors.light) {
      sensors.light->publish_state(NAN);
    }
    this->sensors_.erase(address);
  }
}

void ChirpComponent::create_sensors_(uint8_t address, ChirpDevice *device) {
  ESP_LOGD(TAG, "Creating sensors for device at 0x%02X", address);
  
  ChirpSensors sensors;
  std::string device_name = device->get_display_name();
  
  // Create moisture sensor
  sensors.moisture = new sensor::Sensor();
  sensors.moisture->set_name(this->get_sensor_name_("Soil Moisture", device_name).c_str());
  sensors.moisture->set_unit_of_measurement("%");
  sensors.moisture->set_device_class("moisture");
  sensors.moisture->set_icon("mdi:water-percent");
  sensors.moisture->set_accuracy_decimals(0);
  App.register_sensor(sensors.moisture);
  
  // Create temperature sensor
  sensors.temperature = new sensor::Sensor();
  sensors.temperature->set_name(this->get_sensor_name_("Soil Temperature", device_name).c_str());
  sensors.temperature->set_unit_of_measurement("°C");
  sensors.temperature->set_device_class("temperature");
  sensors.temperature->set_icon("mdi:thermometer");
  sensors.temperature->set_accuracy_decimals(1);
  App.register_sensor(sensors.temperature);
  
  // Create light sensor
  sensors.light = new sensor::Sensor();
  sensors.light->set_name(this->get_sensor_name_("Soil Light", device_name).c_str());
  sensors.light->set_unit_of_measurement("lx");
  sensors.light->set_device_class("illuminance");
  sensors.light->set_icon("mdi:white-balance-sunny");
  sensors.light->set_accuracy_decimals(0);
  App.register_sensor(sensors.light);
  
  this->sensors_[address] = sensors;
  
  ESP_LOGD(TAG, "Sensors created for device %s", device_name.c_str());
}

void ChirpComponent::update_device_(ChirpDevice *device) {
  uint8_t address = device->get_address();
  
  if (this->sensors_.count(address) == 0) {
    return;  // No sensors for this device
  }
  
  auto &sensors = this->sensors_[address];
  uint32_t now = millis();
  
  // Read moisture
  auto capacitance = device->read_capacitance();
  if (capacitance.has_value()) {
    float moisture = this->capacitance_to_moisture_(capacitance.value());
    if (sensors.moisture) {
      sensors.moisture->publish_state(moisture);
    }
    ESP_LOGV(TAG, "Device 0x%02X - Capacitance: %d, Moisture: %.0f%%", 
             address, capacitance.value(), moisture);
  }
  
  // Read temperature
  auto temp_raw = device->read_temperature();
  if (temp_raw.has_value()) {
    float temperature = this->raw_to_temperature_(temp_raw.value());
    if (sensors.temperature) {
      sensors.temperature->publish_state(temperature);
    }
    ESP_LOGV(TAG, "Device 0x%02X - Temperature: %.1f°C", address, temperature);
  }
  
  // Handle light measurement (requires request and delay)
  if (!sensors.light_requested) {
    // Request light measurement
    if (device->request_light_measurement()) {
      sensors.light_requested = true;
      sensors.last_light_request = now;
    }
  } else if (now - sensors.last_light_request >= LIGHT_MEASUREMENT_DELAY) {
    // Read light measurement
    auto light = device->read_light();
    if (light.has_value()) {
      if (sensors.light) {
        sensors.light->publish_state(light.value());
      }
      ESP_LOGV(TAG, "Device 0x%02X - Light: %d lx", address, light.value());
    }
    sensors.light_requested = false;
  }
}

void ChirpComponent::load_labels_() {
  // Use ESPHome preferences to load labels
  this->pref_ = global_preferences->make_preference<LabelStorage>(fnv1_hash("chirp_labels"));
  
  // Try to load labels for all possible addresses
  for (uint8_t addr = this->scan_start_; addr <= this->scan_end_; addr++) {
    LabelStorage storage;
    if (this->pref_.load(&storage)) {
      if (storage.address == addr && strlen(storage.label) > 0) {
        // Find device with this address
        for (auto *dev : this->devices_) {
          if (dev->get_address() == addr) {
            dev->set_label(std::string(storage.label));
            ESP_LOGD(TAG, "Loaded label '%s' for device 0x%02X", storage.label, addr);
            break;
          }
        }
      }
    }
  }
}

void ChirpComponent::save_labels_() {
  // Save labels for all devices
  for (auto *device : this->devices_) {
    if (device->has_label()) {
      LabelStorage storage;
      storage.address = device->get_address();
      strncpy(storage.label, device->get_label().c_str(), sizeof(storage.label) - 1);
      storage.label[sizeof(storage.label) - 1] = '\0';
      
      auto pref = global_preferences->make_preference<LabelStorage>(
          fnv1_hash(str_sprintf("chirp_label_%02X", device->get_address()).c_str()));
      pref.save(&storage);
      
      ESP_LOGD(TAG, "Saved label '%s' for device 0x%02X", storage.label, device->get_address());
    }
  }
}

std::string ChirpComponent::get_sensor_name_(const std::string &sensor_type, const std::string &device_name) {
  return sensor_type + " - " + device_name;
}

float ChirpComponent::capacitance_to_moisture_(uint16_t capacitance) {
  // Use calibration from original config
  float moisture = (static_cast<float>(capacitance) - MOISTURE_DRY) / (MOISTURE_WET - MOISTURE_DRY) * 100.0f;
  
  // Clamp to 0-100%
  if (moisture < 0.0f) moisture = 0.0f;
  if (moisture > 100.0f) moisture = 100.0f;
  
  return moisture;
}

float ChirpComponent::raw_to_temperature_(uint16_t raw_temp) {
  // Temperature is in 0.1°C units
  return static_cast<float>(raw_temp) / 10.0f;
}

}  // namespace chirp
}  // namespace esphome

