# Plantsensor - Multi-Device Chirp Soil Moisture Sensor

ESPHome configuration for automatically discovering and managing multiple Chirp I2C soil moisture sensors.

## Features

✅ **Auto-Discovery**: Automatically detects Chirp sensors on the I2C bus  
✅ **Dynamic Sensors**: Creates Home Assistant sensors for each device found  
✅ **Device Labeling**: Assign friendly names via Home Assistant services  
✅ **Address Management**: Change I2C addresses remotely  
✅ **Persistent Storage**: Labels persist across reboots  
✅ **Hot-Plug Support**: Detects new devices without reflashing  

## Quick Start

### 1. Flash Configuration

```bash
esphome run plantsensor.yaml
```

### 2. View Sensors in Home Assistant

Sensors will appear automatically:
- `sensor.soil_moisture_<address>`
- `sensor.soil_temperature_<address>`
- `sensor.soil_light_<address>`

### 3. Label Your Devices

In Home Assistant Developer Tools > Services:

```yaml
service: esphome.plantsensor_jer_set_label
data:
  address: 0x6F
  label: "Tomato Plant"
```

## File Structure

```
Plantsensor/
├── plantsensor.yaml              # Main ESPHome configuration
├── components/
│   └── chirp/                    # Custom component
│       ├── __init__.py           # Python config schema
│       ├── chirp.h               # Main component header
│       ├── chirp.cpp             # Main component implementation
│       ├── chirp_device.h        # Device handler header
│       └── chirp_device.cpp      # Device handler implementation
├── CHIRP_USAGE.md                # Detailed usage guide
├── TESTING_CHECKLIST.md          # Testing procedures
└── README.md                     # This file
```

## Hardware Requirements

- **ESP32** development board (tested on NodeMCU-32S)
- **Chirp I2C Soil Moisture Sensors** (one or more)
- **I2C Wiring**:
  - SDA: GPIO21
  - SCL: GPIO22
  - Pull-up resistors: 4.7kΩ (usually built-in)

## Component Details

### Custom Chirp Component

The `chirp` component is a custom ESPHome component that:

1. **Scans** the I2C bus for Chirp devices (0x01-0x7F)
2. **Validates** devices by reading known Chirp registers
3. **Creates** sensor entities dynamically for each device:
   - Moisture (% with device_class: moisture)
   - Temperature (°C with device_class: temperature)
   - Light (lx with device_class: illuminance)
4. **Manages** device addressing and labeling via Home Assistant services
5. **Stores** labels persistently in ESP32 flash memory

### Calibration

Uses calibration from original configuration:
- **Dry**: 263 (raw capacitance in air)
- **Wet**: 483 (raw capacitance in water)

Moisture percentage is calculated as:
```
moisture = (capacitance - 263) / (483 - 263) × 100%
```

## Available Services

### Set Device Label

Assigns a friendly name to a device.

```yaml
service: esphome.plantsensor_jer_set_label
data:
  address: 0x6F  # Device I2C address
  label: "Tomato Plant"  # Friendly name
```

### Change Device Address

Changes the I2C address of a Chirp sensor.

```yaml
service: esphome.plantsensor_jer_set_address
data:
  old_address: 0x20  # Current address
  new_address: 0x21  # New address
```

⚠️ **Warning**: This permanently changes the sensor's address.

### Rescan I2C Bus

Forces an immediate scan for new devices.

```yaml
service: esphome.plantsensor_jer_rescan
```

## Adding Multiple Sensors

### Typical Workflow

1. **First sensor** (existing at 0x6F):
   ```yaml
   # Auto-detected on boot
   # Label it: set_label(0x6F, "Plant 1")
   ```

2. **Second sensor** (default 0x20):
   ```yaml
   # Connect and wait for auto-scan or call rescan
   # Change address: set_address(0x20, 0x21)
   # Label it: set_label(0x21, "Plant 2")
   ```

3. **Third sensor** (default 0x20):
   ```yaml
   # Connect and rescan
   # Change address: set_address(0x20, 0x22)
   # Label it: set_label(0x22, "Plant 3")
   ```

Each sensor adds 3 entities to Home Assistant (moisture, temperature, light).

## Configuration Options

```yaml
chirp:
  i2c_id: bus_a                    # I2C bus ID
  scan_interval: 60s               # Auto-scan frequency (default: 60s)
  address_range:                   # Optional address range
    start: 0x01                    # Start address (default: 0x01)
    end: 0x7F                      # End address (default: 0x7F)
```

## Documentation

- **[CHIRP_USAGE.md](CHIRP_USAGE.md)**: Complete usage guide with examples
- **[TESTING_CHECKLIST.md](TESTING_CHECKLIST.md)**: Testing procedures and verification

## Troubleshooting

### Device Not Detected

- Check I2C wiring (SDA: GPIO21, SCL: GPIO22)
- Verify power to sensors (3.3V or 5V)
- Check ESPHome logs for I2C errors
- Try manual rescan service

### Address Change Failed

- Verify old address is correct
- Ensure new address is not in use
- Check address is valid (0x01-0x7F)
- Review logs for specific error

### Sensors Unavailable

- Check device hasn't been disconnected
- Look for I2C errors in logs
- Call rescan service
- Reboot ESP32 if needed

## Home Assistant Example Automation

```yaml
automation:
  - alias: "Low Moisture Alert"
    trigger:
      - platform: numeric_state
        entity_id: sensor.soil_moisture_tomato_plant
        below: 30
    action:
      - service: notify.mobile_app
        data:
          title: "Plant Alert"
          message: "Tomato plant needs watering! ({{ states('sensor.soil_moisture_tomato_plant') }}%)"
```

## Technical Specifications

- **Update Rate**: 
  - Moisture/Temperature: ~5 seconds
  - Light: ~6 seconds (1s measurement delay)
- **Scan Rate**: Every 60 seconds (configurable)
- **Supported Devices**: Up to ~120 (limited by I2C address space)
- **Storage**: Labels stored in ESP32 NVS flash
- **I2C Speed**: 100 kHz (standard mode)

## Credits

- **Original Config**: Based on Chirp sensor integration at https://wemakethings.net/chirp/
- **Component**: Custom ESPHome component for multi-device support
- **Calibration**: Original calibration values (dry: 263, wet: 483)

## License

This project configuration and custom component are provided as-is for use with ESPHome and Home Assistant.

