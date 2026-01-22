/**
 * @file mh_x25.h
 * @author Matthias Hefel
 * @date 2026
 * @brief MH X25 LED Moving Head Light Control Library
 *
 * This library provides a high-level interface for controlling the MH X25
 * LED moving head light in 12-channel DMX mode.
 *
 * 12-Channel Mode DMX Mapping:
 * - Channel 1: Pan (coarse)
 * - Channel 2: Tilt (coarse)
 * - Channel 3: Pan (fine)
 * - Channel 4: Tilt (fine)
 * - Channel 5: Pan/Tilt Speed
 * - Channel 6: Color
 * - Channel 7: Shutter
 * - Channel 8: Dimmer
 * - Channel 9: Gobo
 * - Channel 10: Gobo Rotation
 * - Channel 11: Special Functions
 * - Channel 12: Built-in Programs
 */

#ifndef MH_X25_H
#define MH_X25_H

#include <stdint.h>
#include "esp_err.h"
#include "dmx_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* MH X25 Channel Definitions (12-channel mode) */
#define MH_X25_CHANNEL_PAN 0       // Channel 1: Pan (coarse)
#define MH_X25_CHANNEL_TILT 1      // Channel 2: Tilt (coarse)
#define MH_X25_CHANNEL_PAN_FINE 2  // Channel 3: Pan (fine)
#define MH_X25_CHANNEL_TILT_FINE 3 // Channel 4: Tilt (fine)
#define MH_X25_CHANNEL_SPEED 4     // Channel 5: Pan/Tilt Speed
#define MH_X25_CHANNEL_COLOR 5     // Channel 6: Color wheel
#define MH_X25_CHANNEL_SHUTTER 6   // Channel 7: Shutter/Strobe
#define MH_X25_CHANNEL_DIMMER 7    // Channel 8: Dimmer
#define MH_X25_CHANNEL_GOBO 8      // Channel 9: Gobo wheel
#define MH_X25_CHANNEL_GOBO_ROT 9  // Channel 10: Gobo rotation
#define MH_X25_CHANNEL_SPECIAL 10  // Channel 11: Special Functions
#define MH_X25_CHANNEL_PROGRAM 11  // Channel 12: Built-in Programs

#define MH_X25_NUM_CHANNELS 12

/* Pan/Tilt Speed Values (Channel 5) */
#define MH_X25_SPEED_FAST 0   // Fast movement
#define MH_X25_SPEED_SLOW 255 // Slow movement

/* Color Wheel Values (Channel 6) - According to datasheet */
#define MH_X25_COLOR_WHITE 0         // 0-7 White
#define MH_X25_COLOR_YELLOW 7        // 5-9 Yellow
#define MH_X25_COLOR_PINK 12         // 10-14 Pink
#define MH_X25_COLOR_GREEN 17        // 15-19 Green
#define MH_X25_COLOR_PEACHBLOW 22    // 20-24 Peachblow
#define MH_X25_COLOR_LIGHT_BLUE 27   // 25-29 Light blue
#define MH_X25_COLOR_YELLOW_GREEN 32 // 30-34 Yellow green
#define MH_X25_COLOR_RED 37          // 35-39 Red
#define MH_X25_COLOR_DARK_BLUE 42    // 40-44 Dark blue
#define MH_X25_COLOR_RAINBOW_CW 160  // 128-191 Rainbow clockwise (mid-range)
#define MH_X25_COLOR_RAINBOW_CCW 224 // 192-255 Rainbow counter-clockwise (mid-range)

/* Shutter/Strobe Values (Channel 7) - According to datasheet */
#define MH_X25_SHUTTER_BLACKOUT 0      // 0-3 Blackout
#define MH_X25_SHUTTER_OPEN 7          // 4-7 and 216-255 Open
#define MH_X25_SHUTTER_STROBE_SLOW 50  // 8-215 Strobe effect, increasing speed
#define MH_X25_SHUTTER_STROBE_MED 130  // 8-215 Strobe effect, increasing speed
#define MH_X25_SHUTTER_STROBE_FAST 200 // 8-215 Strobe effect, increasing speed

/* Dimmer Values (Channel 8) */
#define MH_X25_DIMMER_OFF 0
#define MH_X25_DIMMER_FULL 255

/* Gobo Wheel Values (Channel 9) - According to datasheet */
#define MH_X25_GOBO_OPEN 0          // 0-7 Open
#define MH_X25_GOBO_2 12            // 8-15 Gobo 2
#define MH_X25_GOBO_3 20            // 16-23 Gobo 3
#define MH_X25_GOBO_4 28            // 24-31 Gobo 4
#define MH_X25_GOBO_5 36            // 32-39 Gobo 5
#define MH_X25_GOBO_6 44            // 40-47 Gobo 6
#define MH_X25_GOBO_7 52            // 48-55 Gobo 7
#define MH_X25_GOBO_8 60            // 56-63 Gobo 8
#define MH_X25_GOBO_8_SHAKE 68      // 64-71 Gobo 8 shake
#define MH_X25_GOBO_7_SHAKE 76      // 72-79 Gobo 7 shake
#define MH_X25_GOBO_RAINBOW_CW 160  // 128-191 Rainbow clockwise
#define MH_X25_GOBO_RAINBOW_CCW 224 // 192-255 Rainbow counter-clockwise

/* Gobo Rotation Values (Channel 10) - According to datasheet */
#define MH_X25_GOBO_ROT_STOP 32      // 0-63 Fixed position (mid-range for ~180°)
#define MH_X25_GOBO_ROT_CW_SLOW 80   // 64-147 Rotation clockwise slow
#define MH_X25_GOBO_ROT_CW_FAST 130  // 64-147 Rotation clockwise fast
#define MH_X25_GOBO_ROT_CCW_SLOW 180 // 148-231 Rotation counter-clockwise slow
#define MH_X25_GOBO_ROT_CCW_FAST 220 // 148-231 Rotation counter-clockwise fast

/* Special Functions Values (Channel 11) - According to datasheet */
#define MH_X25_SPECIAL_NONE 0                   // 0-7 Not in use
#define MH_X25_SPECIAL_BLACKOUT_PAN_TILT 12     // 8-15 Blackout during pan/tilt
#define MH_X25_SPECIAL_NO_BLACKOUT_PAN_TILT 20  // 16-23 No blackout during pan/tilt
#define MH_X25_SPECIAL_BLACKOUT_COLOR 28        // 24-31 Blackout during color change
#define MH_X25_SPECIAL_NO_BLACKOUT_COLOR 36     // 32-39 No blackout during color change
#define MH_X25_SPECIAL_BLACKOUT_GOBO 44         // 40-47 Blackout during gobo change
#define MH_X25_SPECIAL_NO_BLACKOUT_GOBO 52      // 48-55 No blackout during gobo change
#define MH_X25_SPECIAL_BLACKOUT_ALL_MOVEMENT 92 // 88-95 Blackout during movement
#define MH_X25_SPECIAL_RESET_PAN_TILT 100       // 96-103 Pan and tilt reset
#define MH_X25_SPECIAL_RESET_COLOR 116          // 112-119 Color wheel reset
#define MH_X25_SPECIAL_RESET_GOBO 124           // 120-127 Gobo wheel reset
#define MH_X25_SPECIAL_RESET_GOBO_ROT 132       // 128-135 Gobo rotation reset
#define MH_X25_SPECIAL_RESET_ALL 156            // 152-159 All channel reset

    /**
     * @brief MH X25 Device Configuration
     */
    typedef struct
    {
        dmx_handle_t dmx_handle; ///< DMX driver handle
        uint16_t start_channel;  ///< DMX start channel (1-501, allows for 12 channels)
    } mh_x25_config_t;

    /**
     * @brief MH X25 device handle
     */
    typedef void *mh_x25_handle_t;

    /**
     * @brief Initialize MH X25 device
     *
     * @param config Pointer to configuration structure
     * @param out_handle Pointer to store the device handle
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     *      - ESP_ERR_NO_MEM: Out of memory
     */
    esp_err_t mh_x25_init(const mh_x25_config_t *config, mh_x25_handle_t *out_handle);

    /**
     * @brief Deinitialize MH X25 device
     *
     * @param handle Device handle
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid handle
     */
    esp_err_t mh_x25_deinit(mh_x25_handle_t handle);

    /**
     * @brief Set pan (horizontal rotation) position using 8-bit value
     *
     * @param handle Device handle
     * @param pan Pan value (0-255, maps to 0° to max pan range)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_pan(mh_x25_handle_t handle, uint8_t pan);

    /**
     * @brief Set tilt (vertical inclination) position using 8-bit value
     *
     * @param handle Device handle
     * @param tilt Tilt value (0-255, maps to 0° to max tilt range)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_tilt(mh_x25_handle_t handle, uint8_t tilt);

    /**
     * @brief Set both pan and tilt together using 8-bit values
     *
     * @param handle Device handle
     * @param pan Pan value (0-255)
     * @param tilt Tilt value (0-255)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_position(mh_x25_handle_t handle, uint8_t pan, uint8_t tilt);

    /**
     * @brief Set both pan and tilt together using 16-bit values for high precision
     *
     * @param handle Device handle
     * @param pan_16bit Pan value (0-65535)
     * @param tilt_16bit Tilt value (0-65535)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_position_16bit(mh_x25_handle_t handle, uint16_t pan_16bit, uint16_t tilt_16bit);

    /**
     * @brief Set pan/tilt movement speed
     *
     * @param handle Device handle
     * @param speed Speed value (0=fast, 255=slow)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_speed(mh_x25_handle_t handle, uint8_t speed);

    /**
     * @brief Set color wheel position
     *
     * @param handle Device handle
     * @param color Color value (see MH_X25_COLOR_* macros)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_color(mh_x25_handle_t handle, uint8_t color);

    /**
     * @brief Set shutter state
     *
     * @param handle Device handle
     * @param shutter Shutter value (see MH_X25_SHUTTER_* macros)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_shutter(mh_x25_handle_t handle, uint8_t shutter);

    /**
     * @brief Set dimmer level
     *
     * @param handle Device handle
     * @param dimmer Dimmer value (0-255)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_dimmer(mh_x25_handle_t handle, uint8_t dimmer);

    /**
     * @brief Set gobo wheel position
     *
     * @param handle Device handle
     * @param gobo Gobo value (see MH_X25_GOBO_* macros)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_gobo(mh_x25_handle_t handle, uint8_t gobo);

    /**
     * @brief Set gobo rotation
     *
     * @param handle Device handle
     * @param rotation Rotation value (see MH_X25_GOBO_ROT_* macros)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_gobo_rotation(mh_x25_handle_t handle, uint8_t rotation);

    /**
     * @brief Set special function
     *
     * @param handle Device handle
     * @param special Special function value
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_special(mh_x25_handle_t handle, uint8_t special);

    /**
     * @brief Turn off the light (shutter and dimmer)
     *
     * @param handle Device handle
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid handle
     */
    esp_err_t mh_x25_off(mh_x25_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // MH_X25_DRIVER_H
