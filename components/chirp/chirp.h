#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif
#include "chirp_device.h"
#include <vector>
#include <map>

namespace esphome {
namespace chirp {

/**
 * Structure to hold sensor entities for a single Chirp device.
 */
struct ChirpSensors {
  sensor::Sensor *moisture{nullptr};
  sensor::Sensor *temperature{nullptr};
  sensor::Sensor *light{nullptr};
  uint32_t last_light_request{0};  // Timestamp of last light measurement request
  bool light_requested{false};     // Whether we're waiting for light measurement
};

/**
 * Main component that manages multiple Chirp soil moisture sensors.
 * Handles automatic device discovery, dynamic sensor creation, and persistent labeling.
 */
class ChirpComponent : public Component
#ifdef USE_API
    , public api::CustomAPIDevice
#endif
{
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  /**
   * Set the I2C bus to use.
   */
  void set_i2c_bus(i2c::I2CBus *bus) { this->i2c_bus_ = bus; }

  /**
   * Set the scan interval in milliseconds.
   */
  void set_scan_interval(uint32_t interval) { this->scan_interval_ = interval; }

  /**
   * Set the I2C address range to scan.
   */
  void set_address_range(uint8_t start, uint8_t end) {
    this->scan_start_ = start;
    this->scan_end_ = end;
  }

  /**
   * Service: Change the I2C address of a device.
   */
  void set_device_address(uint8_t old_address, uint8_t new_address);

  /**
   * Service: Set a friendly label for a device.
   */
  void set_device_label(uint8_t address, const std::string &label);

  /**
   * Service: Force a rescan of the I2C bus.
   */
  void rescan_bus();

 protected:
  i2c::I2CBus *i2c_bus_{nullptr};
  uint32_t scan_interval_{60000};  // 60 seconds default
  uint8_t scan_start_{0x01};
  uint8_t scan_end_{0x7F};
  uint32_t last_scan_{0};
  uint32_t update_counter_{0};

  std::vector<ChirpDevice *> devices_;
  std::map<uint8_t, ChirpSensors> sensors_;

  ESPPreferenceObject pref_;

  /**
   * Scan the I2C bus for Chirp devices.
   */
  void scan_for_devices_();

  /**
   * Add a newly discovered device.
   */
  void add_device_(uint8_t address);

  /**
   * Remove a device that's no longer responding.
   */
  void remove_device_(uint8_t address);

  /**
   * Create sensor entities for a device.
   */
  void create_sensors_(uint8_t address, ChirpDevice *device);

  /**
   * Update sensor values for a device.
   */
  void update_device_(ChirpDevice *device);

  /**
   * Load stored labels from flash.
   */
  void load_labels_();

  /**
   * Save labels to flash.
   */
  void save_labels_();

  /**
   * Get sensor name with label.
   */
  std::string get_sensor_name_(const std::string &sensor_type, const std::string &device_name);

  /**
   * Convert raw capacitance to moisture percentage.
   * Uses calibration values: dry=263, wet=483 (from original config).
   */
  float capacitance_to_moisture_(uint16_t capacitance);

  /**
   * Convert raw temperature value to degrees Celsius.
   */
  float raw_to_temperature_(uint16_t raw_temp);
};

}  // namespace chirp
}  // namespace esphome

