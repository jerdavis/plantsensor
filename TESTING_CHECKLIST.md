# Chirp Component Testing Checklist

This checklist helps verify the custom Chirp component works correctly with single and multiple devices.

## Prerequisites

- [ ] ESP32 device with ESPHome configured
- [ ] One or more Chirp I2C soil moisture sensors
- [ ] I2C wiring: SDA to GPIO21, SCL to GPIO22
- [ ] Pull-up resistors on I2C lines (if not built-in)
- [ ] Home Assistant connected to ESPHome device

## Test 1: Single Device Detection

### Objective
Verify the component detects and reads from a single Chirp sensor.

### Steps

1. [ ] Connect one Chirp sensor to I2C bus (existing sensor at 0x6F)
2. [ ] Compile and flash the configuration:
   ```bash
   esphome run plantsensor.yaml
   ```
3. [ ] Monitor the logs during boot
4. [ ] Verify log shows:
   ```
   [I][chirp:xxx] Setting up Chirp component...
   [I][chirp:xxx] Chirp device found at address 0x6F
   [I][chirp:xxx] Chirp component setup complete. Found 1 device(s)
   ```

### Expected Results

- [ ] Component detects 1 device
- [ ] No I2C errors in logs
- [ ] Three sensors appear in Home Assistant:
  - `sensor.soil_moisture_0x6f`
  - `sensor.soil_temperature_0x6f`
  - `sensor.soil_light_0x6f`
- [ ] All sensors show valid values (not NaN or unavailable)
- [ ] Moisture reading is between 0-100%
- [ ] Temperature reading is reasonable (e.g., 15-30°C)
- [ ] Light reading is a positive number

### Verification

```bash
# Check sensor values in Home Assistant
# Developer Tools > States
# Look for: sensor.soil_moisture_0x6f, sensor.soil_temperature_0x6f, sensor.soil_light_0x6f
```

## Test 2: Device Labeling

### Objective
Verify labeling persists across reboots.

### Steps

1. [ ] In Home Assistant Developer Tools > Services, call:
   ```yaml
   service: esphome.plantsensor_jer_set_label
   data:
     address: 0x6F
     label: "Test Plant 1"
   ```
2. [ ] Verify log shows:
   ```
   [I][chirp:xxx] Service call: set_device_label(0x6F, 'Test Plant 1')
   [I][chirp:xxx] Successfully set label for device at 0x6F
   ```
3. [ ] Check Home Assistant - sensor names should update:
   - `sensor.soil_moisture_test_plant_1`
   - `sensor.soil_temperature_test_plant_1`
   - `sensor.soil_light_test_plant_1`
4. [ ] Reboot ESP32 device
5. [ ] Verify sensors still have labeled names after reboot

### Expected Results

- [ ] Service call succeeds
- [ ] Sensor entity names update immediately
- [ ] Label persists after reboot
- [ ] Sensor history is preserved (same entity IDs)

## Test 3: Auto-Discovery Scan

### Objective
Verify automatic scanning detects new devices.

### Steps

1. [ ] Note current device count in logs
2. [ ] Connect a second Chirp sensor (will have default address 0x20)
3. [ ] Wait 60 seconds for automatic rescan
4. [ ] Check logs for new device detection:
   ```
   [I][chirp:xxx] Adding new Chirp device at address 0x20
   ```
5. [ ] Verify 6 new sensors appear in Home Assistant (3 for the new device)

### Alternative: Manual Rescan

1. [ ] Connect second Chirp sensor
2. [ ] Immediately call rescan service:
   ```yaml
   service: esphome.plantsensor_jer_rescan
   ```
3. [ ] Check logs for immediate detection

### Expected Results

- [ ] Second device detected automatically or via rescan
- [ ] Total of 6 sensors in Home Assistant
- [ ] New sensors named with 0x20 address
- [ ] Both devices reading values correctly

## Test 4: Address Change

### Objective
Verify I2C address can be changed via service.

### Steps

1. [ ] Ensure device at 0x20 is detected
2. [ ] Call set_address service:
   ```yaml
   service: esphome.plantsensor_jer_set_address
   data:
     old_address: 0x20
     new_address: 0x21
   ```
3. [ ] Verify log shows:
   ```
   [I][chirp:xxx] Service call: set_device_address(0x20 -> 0x21)
   [I][chirp:xxx] Changing I2C address from 0x20 to 0x21
   [I][chirp:xxx] Successfully changed address to 0x21
   ```
4. [ ] Check that sensors update:
   - Old sensors (0x20) become unavailable
   - New sensors (0x21) appear and show values

### Expected Results

- [ ] Address change succeeds
- [ ] Device responds at new address
- [ ] Sensors automatically update to new address
- [ ] No I2C errors with the new address
- [ ] Can now add another device at 0x20

## Test 5: Multiple Devices

### Objective
Verify multiple devices work simultaneously with unique labels.

### Setup: 3 Devices

1. [ ] Device 1 at 0x6F labeled "Plant 1"
2. [ ] Device 2 at 0x21 labeled "Plant 2"  
3. [ ] Device 3 (new) at 0x20, change to 0x22, label "Plant 3"

### Steps

1. [ ] Start with devices 1 and 2 already configured (from previous tests)
2. [ ] Connect third sensor (address 0x20)
3. [ ] Rescan to detect third device
4. [ ] Change address: `set_address(0x20, 0x22)`
5. [ ] Label it: `set_label(0x22, "Plant 3")`
6. [ ] Verify all 9 sensors (3 devices × 3 sensors each) show in Home Assistant

### Expected Results

- [ ] All 3 devices detected
- [ ] Total of 9 sensors in Home Assistant
- [ ] Each device has unique label
- [ ] All sensors reading independently
- [ ] No I2C communication errors
- [ ] Reasonable values from all sensors

### Verification

Monitor logs for a few minutes to ensure stable operation:
- [ ] No repeated I2C errors
- [ ] All sensor values updating regularly
- [ ] No sensor values stuck or frozen

## Test 6: Error Handling

### Objective
Verify component handles errors gracefully.

### Test 6a: Invalid Address Change

1. [ ] Try to change to an invalid address:
   ```yaml
   service: esphome.plantsensor_jer_set_address
   data:
     old_address: 0x21
     new_address: 0xFF  # Invalid (too high)
   ```
2. [ ] Expected: Error log, address unchanged

### Test 6b: Address Conflict

1. [ ] Try to change to an existing address:
   ```yaml
   service: esphome.plantsensor_jer_set_address
   data:
     old_address: 0x21
     new_address: 0x6F  # Already in use
   ```
2. [ ] Expected: Error log "Address 0x6F is already in use"

### Test 6c: Device Disconnection

1. [ ] Physically disconnect one sensor
2. [ ] Wait for next scan (up to 60s)
3. [ ] Expected: Device removed from active list, sensors show unavailable

### Expected Results

- [ ] Invalid operations are rejected with clear error messages
- [ ] System remains stable after errors
- [ ] Other devices continue working normally

## Test 7: Persistence

### Objective
Verify labels and addresses persist across power cycles.

### Steps

1. [ ] Configure 2-3 devices with unique addresses and labels
2. [ ] Note the configuration (addresses and labels)
3. [ ] Power off ESP32 completely (not just reset)
4. [ ] Power on ESP32
5. [ ] Verify all devices detected with correct labels
6. [ ] Check sensor entities have same names

### Expected Results

- [ ] All devices rediscovered on boot
- [ ] Labels restored from flash memory
- [ ] Address changes are permanent on the sensors
- [ ] Home Assistant sensor entities unchanged

## Test 8: Calibration Verification

### Objective
Verify moisture readings are calibrated correctly.

### Steps

1. [ ] Remove sensor from soil (dry air)
2. [ ] Note capacitance raw value in logs
3. [ ] Expected: ~263 (from calibration)
4. [ ] Place sensor in water
5. [ ] Note capacitance raw value
6. [ ] Expected: ~483 (from calibration)
7. [ ] Verify moisture shows:
   - 0% in dry air
   - 100% in water
   - Intermediate values in soil

### Expected Results

- [ ] Dry air reads close to 0%
- [ ] Water reads close to 100%
- [ ] Soil readings are reasonable (20-80%)
- [ ] No negative percentages
- [ ] No values above 100%

## Summary Checklist

### Core Functionality
- [ ] Single device detection works
- [ ] Multiple device detection works
- [ ] Sensors read correct values
- [ ] Auto-scan discovers new devices

### Services
- [ ] set_label service works and persists
- [ ] set_address service changes address
- [ ] rescan service triggers immediate scan

### Robustness
- [ ] Error handling is appropriate
- [ ] Invalid operations rejected safely
- [ ] Disconnected devices handled gracefully
- [ ] Labels persist across reboots
- [ ] Addresses persist (stored on sensor)

### Integration
- [ ] Sensors appear in Home Assistant
- [ ] Sensor names update with labels
- [ ] All device classes correct (moisture, temperature, illuminance)
- [ ] Units of measurement correct (%, °C, lx)
- [ ] Icons display correctly

## Issues Found

Document any issues discovered during testing:

| Test | Issue Description | Severity | Status |
|------|------------------|----------|--------|
|      |                  |          |        |

## Test Results Summary

- **Date Tested:** _____________
- **ESPHome Version:** _____________
- **Number of Devices Tested:** _____________
- **Overall Result:** ☐ Pass ☐ Fail ☐ Partial

**Notes:**

