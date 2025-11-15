# DMX512 and MH X25 Configuration Notes

## Overview

This document describes the correct configuration for controlling the MH X25 LED Moving Head light using DMX512 protocol on the ESP32-C3 Clownfish board.

## Hardware Configuration

### ESP32-C3 Clownfish Board - GPIO Pins

- **GPIO4**: UART1 TX → RS-485 DI (Data Input)
- **GPIO5**: UART1 RX → RS-485 RO (not used in TX-only mode)
- **GPIO6**: DE/RE Control → RS-485 transceiver enable (HIGH = transmit)

**⚠️ CRITICAL**: DO NOT use GPIO18/19 - these are hardwired to USB D-/D+ and using them will disable USB programming!

### RS-485 Transceiver Connection

```
ESP32-C3          RS-485 Module       DMX Connector
GPIO4 (TX)   →    DI (Data In)
GPIO5 (RX)   →    RO (unused)
GPIO6        →    DE/RE (enable)
                  A/B (differential) → XLR Pins 2/3
GND          →    GND                → XLR Pin 1
```

## DMX512-A Protocol Specification

### Electrical

- **Baud Rate**: 250,000 bps (250 kbps)
- **Data Format**: 8N2 (8 data bits, no parity, 2 stop bits)
- **Voltage**: RS-485 differential (2-5V typical)

### Timing

- **Break**: 92µs (spec: 88-1000µs minimum)
- **MAB** (Mark After Break): 12µs (spec: 8-1000µs minimum)
- **Refresh Rate**: 44Hz (spec: 1-44Hz, 44Hz recommended for smooth movement)
- **Frame Time**: ~22.7ms per frame @ 44Hz

### Data Structure

```
[BREAK] [MAB] [Start Code] [Channel 1] [Channel 2] ... [Channel 512]
 92µs    12µs      0x00       0-255       0-255          0-255
```

## MH X25 Moving Head Light Specification

### Device Physical Configuration

Set these on the device menu:

1. **DMX Mode**: `6-CH` (NOT 12-CH or b-CH)
2. **DMX Address**: `d001` (address 1)
3. **Pan Range**: `PA54` (540° total range)
4. **Tilt Range**: `ti27` (270° total range)
5. **Reset**: Execute `rESt` after configuration

### 6-Channel Mode DMX Mapping

#### Channel 1: Pan (Horizontal Rotation)

- **Range**: 0-255
- **Physical Range**: 0-540°
- **Calculation**: `degrees = (value / 255) * 540`
- **Example**: Value 128 = 270° (center)

#### Channel 2: Tilt (Vertical Rotation)

- **Range**: 0-255
- **Physical Range**: 0-270°
- **Calculation**: `degrees = (value / 255) * 270`
- **Example**: Value 128 = 135° (center)

#### Channel 3: Color Wheel

| Value Range | Color        | Code Constant                    |
| ----------- | ------------ | -------------------------------- |
| 0-7         | White        | `MH_X25_COLOR_WHITE` (3)         |
| 5-9         | Yellow       | `MH_X25_COLOR_YELLOW` (7)        |
| 10-14       | Pink         | `MH_X25_COLOR_PINK` (12)         |
| 15-19       | Green        | `MH_X25_COLOR_GREEN` (17)        |
| 20-24       | Peachblow    | `MH_X25_COLOR_PEACHBLOW` (22)    |
| 25-29       | Light Blue   | `MH_X25_COLOR_LIGHT_BLUE` (27)   |
| 30-34       | Yellow Green | `MH_X25_COLOR_YELLOW_GREEN` (32) |
| 35-39       | Red          | `MH_X25_COLOR_RED` (37)          |
| 40-44       | Dark Blue    | `MH_X25_COLOR_DARK_BLUE` (42)    |
| 128-191     | Rainbow CW   | `MH_X25_COLOR_RAINBOW_CW` (160)  |
| 192-255     | Rainbow CCW  | `MH_X25_COLOR_RAINBOW_CCW` (224) |

#### Channel 4: Shutter/Strobe

| Value Range | Function    | Code Constant                      |
| ----------- | ----------- | ---------------------------------- |
| 0-3         | Blackout    | `MH_X25_SHUTTER_BLACKOUT` (1)      |
| 4-7         | Open        | `MH_X25_SHUTTER_OPEN` (5)          |
| 8-215       | Strobe      | `MH_X25_SHUTTER_STROBE_*` (50-200) |
| 216-255     | Open Bright | `MH_X25_SHUTTER_OPEN_BRIGHT` (220) |

**⚠️ IMPORTANT**: Use value 5 for normal operation, NOT 0 or 7!

#### Channel 5: Gobo Wheel (Pattern Selection)

| Value Range | Gobo           | Code Constant                   |
| ----------- | -------------- | ------------------------------- |
| 0-7         | Open (no gobo) | `MH_X25_GOBO_OPEN` (3)          |
| 8-15        | Gobo 2         | `MH_X25_GOBO_2` (12)            |
| 16-23       | Gobo 3         | `MH_X25_GOBO_3` (20)            |
| 24-31       | Gobo 4         | `MH_X25_GOBO_4` (28)            |
| 32-39       | Gobo 5         | `MH_X25_GOBO_5` (36)            |
| 40-47       | Gobo 6         | `MH_X25_GOBO_6` (44)            |
| 48-55       | Gobo 7         | `MH_X25_GOBO_7` (52)            |
| 56-63       | Gobo 8         | `MH_X25_GOBO_8` (60)            |
| 128-191     | Rainbow CW     | `MH_X25_GOBO_RAINBOW_CW` (160)  |
| 192-255     | Rainbow CCW    | `MH_X25_GOBO_RAINBOW_CCW` (224) |

**⚠️ IMPORTANT**: Use value 3 for no pattern, NOT 0!

#### Channel 6: Gobo Rotation

| Value Range | Rotation    | Code Constant                    |
| ----------- | ----------- | -------------------------------- |
| 0-63        | Stop/Fixed  | `MH_X25_GOBO_ROT_STOP` (32)      |
| 64-127      | CW Rotation | `MH_X25_GOBO_ROT_CW_*` (80-120)  |
| 128-191     | CCW Slow    | `MH_X25_GOBO_ROT_CCW_SLOW` (180) |
| 192-255     | CCW Fast    | `MH_X25_GOBO_ROT_CCW_FAST` (220) |

## Code Configuration

### Initialization Example

```c
// Initialize DMX driver
dmx_config_t dmx_config = {
    .uart_num = UART_NUM_1,
    .tx_pin = GPIO_NUM_4,
    .rx_pin = GPIO_NUM_5,
    .enable_pin = GPIO_NUM_6,
    .universe_size = 512
};
dmx_handle_t dmx_handle;
dmx_init(&dmx_config, &dmx_handle);
dmx_start_transmission(dmx_handle);

// Initialize MH X25
mh_x25_config_t light_config = {
    .dmx_handle = dmx_handle,
    .start_channel = 1  // DMX address 1
};
mh_x25_handle_t light_handle;
mh_x25_init(&light_config, &light_handle);

// Set to white light, open shutter, no gobo
mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);        // Value 3
mh_x25_set_shutter(light_handle, MH_X25_SHUTTER_OPEN);     // Value 5
mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);           // Value 3
mh_x25_set_gobo_rotation(light_handle, MH_X25_GOBO_ROT_STOP); // Value 32

// Move to position
mh_x25_set_position(light_handle, 128, 128);  // Center position
```

### Basic Movement Example

```c
// Horizontal sweep (left to right)
for (int pan = 0; pan <= 255; pan += 5) {
    mh_x25_set_pan(light_handle, pan);
    vTaskDelay(pdMS_TO_TICKS(50));  // 50ms delay
}

// Diagonal movement
for (int i = 0; i <= 255; i += 5) {
    mh_x25_set_position(light_handle, i, i);
    vTaskDelay(pdMS_TO_TICKS(50));
}
```

## Common Issues and Solutions

### Issue: Light flashing/strobing

**Cause**: Wrong shutter value (using 0 or 7 instead of 5)  
**Solution**: Use `MH_X25_SHUTTER_OPEN` (value 5)

### Issue: Wrong colors showing

**Cause**: Color value 0 instead of 3 for white  
**Solution**: Use `MH_X25_COLOR_WHITE` (value 3)

### Issue: Unwanted patterns appearing

**Cause**: Gobo value 0 instead of 3  
**Solution**: Use `MH_X25_GOBO_OPEN` (value 3)

### Issue: Light not responding

**Checklist**:

1. Device DMX mode set to `6-CH`
2. Device DMX address matches code (default: 1)
3. RS-485 A/B wires not swapped
4. DMX transmission started: `dmx_start_transmission()`
5. GPIO pins correct (4/5/6, NOT 18/19)

### Issue: USB not working after flash

**Cause**: Used GPIO18 or GPIO19  
**Solution**: Reflash using serial adapter on GPIO0/1, then fix GPIO configuration

## Physical Setup Notes

### Orientation

- **Upright**: Light fixture standing on base, pointing at horizontal surface
- **Pan**: 540° horizontal rotation (more than full circle)
- **Tilt**: 270° vertical rotation (almost full vertical range)

### Coordinate System for 1.5m × 2.0m Table

```
Pan mapping (540° range):
  0-255 = 0-540°
  Value 0   ≈ 0°   (far left)
  Value 128 ≈ 270° (center)
  Value 255 ≈ 540° (far right + 180°)

Tilt mapping (270° range):
  0-255 = 0-270°
  Value 0   ≈ 0°   (straight ahead/up)
  Value 128 ≈ 135° (pointing down)
  Value 255 ≈ 270° (pointing straight down)
```

### Calibration Procedure

1. Set pan=128, tilt=128 (center)
2. Note where light points
3. Adjust pan range to cover table width
4. Adjust tilt range to cover table height
5. Record working pan_min, pan_max, tilt_min, tilt_max values

## Files Modified

1. **`main/dmx.h`**:

   - Updated GPIO pin defaults for ESP32-C3 (4/5/6)
   - Added detailed protocol documentation

2. **`main/dmx.c`**:

   - Added comprehensive header documentation
   - Implementation already correct (no changes needed)

3. **`main/mh_x25.h`**:

   - Fixed header comment (was showing RGB LED mode)
   - Corrected all default values per datasheet
   - Added detailed value ranges and explanations

4. **`main/mh_x25.c`**:
   - Added channel specification documentation
   - Implementation already correct (no changes needed)

## Build and Flash

```bash
# Build
idf.py build

# Flash
idf.py flash

# Monitor
idf.py monitor

# Or all at once
idf.py build flash monitor
```

## References

- DMX512-A Standard (ANSI E1.11)
- MH X25 User Manual (6-Channel Mode)
- ESP32-C3 Technical Reference Manual
- ESP-IDF UART Driver Documentation
