# Quick Reference - Chirp Component

Fast reference for common commands and operations.

## Build & Flash

```bash
# Compile and upload
esphome run plantsensor.yaml

# Just compile (check for errors)
esphome compile plantsensor.yaml

# View logs
esphome logs plantsensor.yaml
```

## Home Assistant Services

### Set Label

```yaml
service: esphome.plantsensor_jer_set_label
data:
  address: 0x6F
  label: "Tomato Plant"
```

### Change Address

```yaml
service: esphome.plantsensor_jer_set_address
data:
  old_address: 0x20
  new_address: 0x21
```

### Rescan Bus

```yaml
service: esphome.plantsensor_jer_rescan
```

## Common I2C Addresses

| Address | Usage |
|---------|-------|
| 0x20 | Default Chirp address (factory) |
| 0x6F | Your current sensor |
| 0x21-0x7E | Available for additional sensors |

## Adding New Sensor Workflow

```yaml
# 1. Connect sensor (will be at 0x20)
# 2. Rescan to detect
service: esphome.plantsensor_jer_rescan

# 3. Change address (if needed)
service: esphome.plantsensor_jer_set_address
data:
  old_address: 0x20
  new_address: 0x22

# 4. Label it
service: esphome.plantsensor_jer_set_label
data:
  address: 0x22
  label: "Basil Plant"
```

## Sensor Entity IDs

### Without Label
- `sensor.soil_moisture_0x6f`
- `sensor.soil_temperature_0x6f`
- `sensor.soil_light_0x6f`

### With Label "Tomato Plant"
- `sensor.soil_moisture_tomato_plant`
- `sensor.soil_temperature_tomato_plant`
- `sensor.soil_light_tomato_plant`

## Configuration Options

```yaml
chirp:
  i2c_id: bus_a
  scan_interval: 60s        # How often to rescan for new devices
  address_range:
    start: 0x01
    end: 0x7F
```

## Troubleshooting Commands

### Enable Verbose Logging

Add to `plantsensor.yaml`:

```yaml
logger:
  level: VERBOSE
  logs:
    chirp: VERBOSE
    chirp.device: VERBOSE
```

### Check I2C Bus

The component automatically logs detected devices:

```
[I][chirp:xxx] Chirp component setup complete. Found 2 device(s)
[I][chirp:xxx]   - Address: 0x6F, Label: Tomato Plant
[I][chirp:xxx]   - Address: 0x21, Label: Basil Plant
```

## File Locations

| File | Purpose |
|------|---------|
| `plantsensor.yaml` | Main ESPHome config |
| `components/chirp/` | Custom component code |
| `README.md` | Project overview |
| `CHIRP_USAGE.md` | Detailed usage guide |
| `TESTING_CHECKLIST.md` | Testing procedures |
| `IMPLEMENTATION_SUMMARY.md` | Technical details |

## Calibration Values

Current values (hardcoded in `chirp.cpp`):
- **Dry**: 263 (raw capacitance)
- **Wet**: 483 (raw capacitance)

To modify: Edit `components/chirp/chirp.cpp` lines 13-14.

## Useful Log Searches

```bash
# Filter for chirp messages only
esphome logs plantsensor.yaml 2>&1 | grep chirp

# Watch for device detection
esphome logs plantsensor.yaml 2>&1 | grep "Adding new"

# Monitor I2C errors
esphome logs plantsensor.yaml 2>&1 | grep "I2C error"
```

## Quick Diagnosis

### "Device not found"
- ✓ Check wiring (SDA: GPIO21, SCL: GPIO22)
- ✓ Check power (3.3V or 5V)
- ✓ Call rescan service
- ✓ Check I2C pull-up resistors (4.7kΩ)

### "Address already in use"
- ✓ Check existing devices with `esphome logs`
- ✓ Choose different address
- ✓ Connect sensors one at a time

### "Sensors show unavailable"
- ✓ Check device still connected
- ✓ Rescan bus
- ✓ Restart ESP32
- ✓ Check logs for I2C errors

## Home Assistant Automation Example

```yaml
automation:
  - alias: "Water Alert"
    trigger:
      platform: numeric_state
      entity_id: sensor.soil_moisture_tomato_plant
      below: 30
    action:
      service: notify.mobile_app
      data:
        message: "Tomato plant needs water ({{ states('sensor.soil_moisture_tomato_plant') }}%)"
```

## Expected Sensor Ranges

| Sensor | Min | Max | Unit | Normal Range |
|--------|-----|-----|------|--------------|
| Moisture | 0 | 100 | % | 20-80% (in soil) |
| Temperature | -40 | 80 | °C | 15-30°C (room) |
| Light | 0 | 65535 | lx | Varies widely |

## Component Status Check

View in ESPHome logs:

```
[C][chirp:xxx] Chirp Component:
[C][chirp:xxx]   Scan Interval: 60000 ms
[C][chirp:xxx]   Address Range: 0x01 - 0x7F
[C][chirp:xxx]   Devices Found: 2
[C][chirp:xxx]     - Address: 0x6F, Label: Tomato Plant
[C][chirp:xxx]     - Address: 0x21, Label: Basil Plant
```

## Pin Reference

| Pin | Function | ESP32 GPIO |
|-----|----------|------------|
| SDA | I2C Data | GPIO21 |
| SCL | I2C Clock | GPIO22 |
| 3V3 | Power | 3.3V |
| GND | Ground | GND |

## Performance Metrics

- **Scan time**: ~1-2 seconds (depends on address range)
- **Update rate**: Moisture/Temp every 5s, Light every 6s
- **Rescan interval**: 60s (configurable)
- **Memory per device**: ~150 bytes

## Support

1. Check [CHIRP_USAGE.md](CHIRP_USAGE.md) for detailed guide
2. Review [TESTING_CHECKLIST.md](TESTING_CHECKLIST.md)
3. See [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) for technical details
4. Check ESPHome logs for errors

