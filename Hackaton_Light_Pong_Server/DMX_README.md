# DMX512 MH X25 Light Control Project

This project implements DMX512 protocol on ESP32-C3 to control the MH X25 LED moving head light via RS-485.

## Hardware Requirements

- **Clownfish ESP32-C3** development board
- **MH X25** LED moving head light (set to 6-channel DMX mode)
- **RS-485 transceiver** module (should be integrated on Clownfish board)
- **DMX cable** (XLR 3-pin or 5-pin)

## Pin Configuration

**IMPORTANT**: You need to verify and adjust the GPIO pins based on your Clownfish ESP32-C3 board schematic!

Default pin configuration in `main/blink_example_main.c`:

```c
#define DMX_TX_PIN          GPIO_NUM_21     // UART TX to RS-485 DI
#define DMX_RX_PIN          GPIO_NUM_20     // UART RX (not used in TX-only)
#define DMX_ENABLE_PIN      GPIO_NUM_19     // RS-485 DE/RE control
```

### How to Find the Correct Pins

1. Check your Clownfish ESP32-C3 schematic/documentation
2. Look for the RS-485 section and identify:
   - **DI (Data In)** - Connect to ESP32 TX pin
   - **DE/RE (Driver Enable/Receiver Enable)** - Connect to ESP32 GPIO for direction control
   - **A & B** - DMX differential pair (connects to DMX cable)

## MH X25 DMX Configuration

The MH X25 light must be configured to **6-channel mode** with the following channel mapping:

| DMX Channel | Function     | Range | Description                          |
| ----------- | ------------ | ----- | ------------------------------------ |
| 1           | Dimmer       | 0-255 | Overall brightness (0=off, 255=full) |
| 2           | Red          | 0-255 | Red color intensity                  |
| 3           | Green        | 0-255 | Green color intensity                |
| 4           | Blue         | 0-255 | Blue color intensity                 |
| 5           | Strobe       | 0-255 | Strobe effect (0=off, 1-255=speed)   |
| 6           | Mode/Program | 0-255 | Operating mode selection             |

### Setting DMX Address on MH X25

1. Power on the MH X25 light
2. Use the control panel to set DMX address to **1** (or your preferred starting channel)
3. Set the light to **6-channel mode**
4. Update `MH_X25_START_CHANNEL` in the code if using a different address

## Project Structure

```
main/
├── blink_example_main.c    # Main application with demo sequences
├── dmx.h                    # DMX512 protocol driver header
├── dmx.c                    # DMX512 protocol driver implementation
├── mh_x25.h                 # MH X25 light control library header
├── mh_x25.c                 # MH X25 light control library implementation
└── CMakeLists.txt           # Build configuration
```

## Building and Flashing

1. **Configure the project**:

   ```bash
   idf.py menuconfig
   ```

   - The project is already configured for ESP32-C3

2. **Build the project**:

   ```bash
   idf.py build
   ```

3. **Flash to ESP32-C3**:
   ```bash
   idf.py -p /dev/ttyACM0 flash monitor
   ```

## Using the DMX Library

### Basic Example

```c
#include "dmx.h"
#include "mh_x25.h"

// Initialize DMX
dmx_config_t dmx_config = {
    .tx_pin = GPIO_NUM_21,
    .rx_pin = GPIO_NUM_20,
    .enable_pin = GPIO_NUM_19,
    .uart_num = UART_NUM_1,
    .universe_size = 512
};
dmx_handle_t dmx_handle;
dmx_init(&dmx_config, &dmx_handle);

// Initialize MH X25
mh_x25_config_t light_config = {
    .dmx_handle = dmx_handle,
    .start_channel = 1  // DMX address
};
mh_x25_handle_t light_handle;
mh_x25_init(&light_config, &light_handle);

// Start continuous DMX transmission (44Hz)
dmx_start_transmission(dmx_handle);

// Control the light
mh_x25_set_preset_color(light_handle, &MH_X25_COLOR_RED);
mh_x25_set_dimmer(light_handle, 255);
mh_x25_set_rgb(light_handle, 255, 128, 0);  // Orange
mh_x25_set_strobe(light_handle, MH_X25_STROBE_MEDIUM);
```

## API Reference

### DMX Driver Functions

- `dmx_init()` - Initialize DMX driver
- `dmx_deinit()` - Cleanup DMX driver
- `dmx_set_channel()` - Set single DMX channel value
- `dmx_set_channels()` - Set multiple DMX channels
- `dmx_transmit()` - Send one DMX packet
- `dmx_start_transmission()` - Start continuous transmission at 44Hz
- `dmx_stop_transmission()` - Stop continuous transmission
- `dmx_clear_all()` - Clear all channels to 0

### MH X25 Control Functions

- `mh_x25_init()` - Initialize MH X25 light
- `mh_x25_deinit()` - Cleanup MH X25 light
- `mh_x25_set_dimmer()` - Set brightness (0-255)
- `mh_x25_set_rgb()` - Set RGB color
- `mh_x25_set_color()` - Set color using RGB structure
- `mh_x25_set_strobe()` - Set strobe effect
- `mh_x25_set_mode()` - Set operating mode
- `mh_x25_set_all()` - Set all parameters at once
- `mh_x25_off()` - Turn light off
- `mh_x25_set_preset_color()` - Quick color preset

### Predefined Colors

- `MH_X25_COLOR_RED`
- `MH_X25_COLOR_GREEN`
- `MH_X25_COLOR_BLUE`
- `MH_X25_COLOR_WHITE`
- `MH_X25_COLOR_YELLOW`
- `MH_X25_COLOR_CYAN`
- `MH_X25_COLOR_MAGENTA`
- `MH_X25_COLOR_ORANGE`
- `MH_X25_COLOR_PURPLE`

## Demo Modes

The example application cycles through several demo modes:

1. **Color Cycle** - Cycles through 9 preset colors
2. **RGB Fade** - Smooth color transitions
3. **Strobe** - Demonstrates strobe effects at different speeds

## Troubleshooting

### Light doesn't respond

1. **Check DMX address**: Ensure MH X25 is set to the correct address
2. **Verify pins**: Double-check GPIO pin assignments match your board
3. **Check RS-485 wiring**:
   - Verify DMX A and B connections
   - Ensure proper DMX termination (120Ω resistor at end of chain)
4. **Monitor serial output**: Check for error messages
5. **DMX mode**: Ensure light is in 6-channel DMX mode

### Flickering or unstable behavior

1. Check RS-485 enable pin is correctly controlling direction
2. Verify baud rate is exactly 250000
3. Check for proper grounding
4. Add DMX termination resistor (120Ω between A and B at last device)

### Build errors

1. Ensure ESP-IDF environment is activated
2. Run `idf.py clean` then `idf.py build`
3. Check that all source files are listed in `main/CMakeLists.txt`

## Technical Details

### DMX512 Protocol Specifications

- **Baud Rate**: 250,000 bps
- **Data Format**: 8N2 (8 data bits, no parity, 2 stop bits)
- **Break**: 88-1000 µs (92 µs used)
- **MAB** (Mark After Break): 8-1000 µs (12 µs used)
- **Update Rate**: 44 Hz (standard refresh rate)
- **Channels**: 512 maximum

### RS-485 Physical Layer

- Differential signaling (A and B lines)
- DE (Driver Enable): High = Transmit, Low = Receive
- RE (Receiver Enable): Low = Receive, High = Disable (usually tied to DE)

## Next Steps

1. **Verify GPIO pins** on your specific Clownfish board
2. **Test with MH X25** light
3. **Customize demo effects** to your needs
4. **Add more fixtures** by creating additional device handles with different start channels
5. **Integrate with sensors** or network control for interactive lighting

## License

This example code is in the Public Domain (or CC0 licensed, at your option).

## References

- [DMX512 Standard (USITT DMX512-A)](https://tsp.esta.org/tsp/documents/docs/ANSI-ESTA_E1-11_2008R2018.pdf)
- [ESP32-C3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf)
- MH X25 Datasheet: `./data_sheets/c_238185_en_online.pdf`
