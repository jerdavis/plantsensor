# Chirp Component Usage Guide

This guide explains how to use the custom Chirp component for auto-discovering and managing multiple Chirp I2C soil moisture sensors.

## Overview

The custom Chirp component automatically:
- Scans the I2C bus for Chirp sensors
- Creates sensors dynamically for each device found
- Provides services to manage device addresses and labels
- Persists labels across reboots

## Initial Setup

### 1. Flash the Configuration

```bash
esphome run plantsensor.yaml
```

The component will automatically detect any Chirp sensors on the I2C bus during boot.

### 2. Check the Logs

After flashing, check the ESPHome logs to see discovered devices:

```
[I][chirp:xxx] Chirp component setup complete. Found 1 device(s)
[I][chirp:xxx]   - Address: 0x6F, Label: (none)
```

### 3. View in Home Assistant

The sensors will appear in Home Assistant with names like:
- `sensor.soil_moisture_0x6F`
- `sensor.soil_temperature_0x6F`
- `sensor.soil_light_0x6F`

## Testing Single Device

1. **Verify Detection**: Ensure your existing sensor at 0x6F is detected
2. **Check Sensor Values**: Monitor the sensor readings in Home Assistant
3. **Add a Label**: Use the service to give it a friendly name

### Setting a Label

In Home Assistant, go to Developer Tools > Services and call:

```yaml
service: esphome.plantsensor_jer_set_label
data:
  address: 0x6F
  label: "Tomato Plant"
```

After setting the label, sensor names will update to:
- `sensor.soil_moisture_tomato_plant`
- `sensor.soil_temperature_tomato_plant`
- `sensor.soil_light_tomato_plant`

Labels persist across reboots.

## Adding Multiple Devices

### Step 1: Connect First Device

If you already have a device at 0x6F, it will be detected automatically.

### Step 2: Add a Second Device

1. **Connect the new sensor** (will have default address 0x20)
2. **Wait for scan** (up to 60 seconds) or call the rescan service:

```yaml
service: esphome.plantsensor_jer_rescan
```

3. **Check logs** for the new device:

```
[I][chirp:xxx] Adding new Chirp device at address 0x20
```

### Step 3: Change Address (If Needed)

If the second device conflicts with an existing address, or you want to organize addresses:

```yaml
service: esphome.plantsensor_jer_set_address
data:
  old_address: 0x20
  new_address: 0x21
```

**WARNING**: This permanently changes the sensor's I2C address until changed again.

### Step 4: Label the New Device

```yaml
service: esphome.plantsensor_jer_set_label
data:
  address: 0x21
  label: "Basil Plant"
```

### Step 5: Repeat for More Devices

You can add up to ~120 devices (limited by I2C address space 0x01-0x7F), each with a unique address.

## Available Services

### `set_address`

Changes the I2C address of a device.

**Parameters:**
- `old_address`: Current address (e.g., `0x20`)
- `new_address`: Desired new address (e.g., `0x21`)

**Example:**
```yaml
service: esphome.plantsensor_jer_set_address
data:
  old_address: 0x20
  new_address: 0x21
```

### `set_label`

Sets a friendly label for a device.

**Parameters:**
- `address`: Device address (e.g., `0x6F`)
- `label`: Friendly name (e.g., `"Tomato Plant"`)

**Example:**
```yaml
service: esphome.plantsensor_jer_set_label
data:
  address: 0x6F
  label: "Tomato Plant"
```

### `rescan`

Forces an immediate rescan of the I2C bus for new devices.

**Example:**
```yaml
service: esphome.plantsensor_jer_rescan
```

## Troubleshooting

### Device Not Detected

1. **Check wiring**: Ensure SDA (GPIO21) and SCL (GPIO22) are connected
2. **Check power**: Chirp sensors need 3.3V or 5V
3. **Check address**: Use I2C scanner to verify device address
4. **Check logs**: Look for I2C errors in ESPHome logs

### Address Change Failed

1. **Verify old address**: Make sure the device is at the address you specified
2. **Check new address**: Must be between 0x01 and 0x7F
3. **Avoid conflicts**: Don't use an address that's already in use
4. **Check logs**: ESPHome will log the reason for failure

### Sensors Show "Unavailable"

1. **Check device power**: Ensure device hasn't been disconnected
2. **Check I2C bus**: Look for I2C errors in logs
3. **Rescan**: Call the rescan service to rediscover devices
4. **Restart**: Reboot the ESP32 to reinitialize

### Multiple Devices Show Same Values

1. **Address conflict**: Two devices have the same I2C address
2. **Change address**: Use `set_address` service to give one device a unique address
3. **Physical isolation**: Connect devices one at a time to assign unique addresses

## Calibration

The component uses the calibration values from your original configuration:
- Dry: 263 (raw capacitance in air)
- Wet: 483 (raw capacitance in water)

These values are hardcoded in `components/chirp/chirp.cpp`. If you need different calibration:

1. Edit `chirp.cpp` lines with `MOISTURE_DRY` and `MOISTURE_WET`
2. Recompile and flash

For per-sensor calibration (future enhancement), the component could be extended to store calibration values in preferences.

## Typical Workflow for Multiple Sensors

### Scenario: Adding 3 Sensors

1. **Start with one sensor** (existing at 0x6F)
   - Flash the new config
   - Label it: `set_label(0x6F, "Plant 1")`

2. **Connect second sensor** (default 0x20)
   - Wait for auto-detection or call `rescan`
   - Change address: `set_address(0x20, 0x21)`
   - Label it: `set_label(0x21, "Plant 2")`

3. **Connect third sensor** (default 0x20)
   - Call `rescan`
   - Change address: `set_address(0x20, 0x22)`
   - Label it: `set_label(0x22, "Plant 3")`

Now you have 3 sensors with unique addresses and labels, each with 3 sensor entities in Home Assistant (9 total sensors).

## Home Assistant Automation Example

```yaml
automation:
  - alias: "Water Plants When Dry"
    trigger:
      - platform: numeric_state
        entity_id: sensor.soil_moisture_tomato_plant
        below: 30
    action:
      - service: notify.mobile_app
        data:
          message: "Tomato plant needs watering! Moisture: {{ states('sensor.soil_moisture_tomato_plant') }}%"
```

## Technical Details

### Sensor Update Frequency

- Moisture and Temperature: Every ~5 seconds
- Light: Every ~6 seconds (requires 1s delay for measurement)

### Device Scan Frequency

- Automatic rescan: Every 60 seconds (configurable in YAML)
- Manual rescan: Call `rescan` service anytime

### Storage

- Labels are stored in ESP32 flash memory using ESPHome Preferences
- Labels persist across reboots and OTA updates
- Address changes are stored on the Chirp sensor itself

### I2C Communication

- Bus speed: 100 kHz (I2C standard mode)
- Pull-up resistors: Usually required (4.7kΩ typical)
- Bus length: Keep wires short for reliability (<1m recommended)

