/**
 * @file mh_x25.h
 * @brief MH X25 LED Moving Head Light Control Library
 *
 * This library provides a high-level interface for controlling the MH X25
 * LED moving head light in 6-channel DMX mode.
 *
 * 6-Channel Mode DMX Mapping:
 * - Channel 1: Dimmer (0-255)
 * - Channel 2: Red (0-255)
 * - Channel 3: Green (0-255)
 * - Channel 4: Blue (0-255)
 * - Channel 5: Strobe (0-255)
 * - Channel 6: Mode/Program (0-255)
 */

#ifndef MH_X25_H
#define MH_X25_H

#include <stdint.h>
#include "esp_err.h"
#include "dmx.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* MH X25 Channel Definitions (6-channel mode) */
#define MH_X25_CHANNEL_PAN 0      // Channel 1: Pan (rotation)
#define MH_X25_CHANNEL_TILT 1     // Channel 2: Tilt (inclination)
#define MH_X25_CHANNEL_COLOR 2    // Channel 3: Color wheel
#define MH_X25_CHANNEL_SHUTTER 3  // Channel 4: Shutter/Strobe
#define MH_X25_CHANNEL_GOBO 4     // Channel 5: Gobo wheel
#define MH_X25_CHANNEL_GOBO_ROT 5 // Channel 6: Gobo rotation

#define MH_X25_NUM_CHANNELS 6

/* Color Wheel Values (Channel 3) */
#define MH_X25_COLOR_WHITE 0         // White
#define MH_X25_COLOR_YELLOW 7        // Yellow
#define MH_X25_COLOR_PINK 12         // Pink
#define MH_X25_COLOR_GREEN 17        // Green
#define MH_X25_COLOR_PEACHBLOW 22    // Peachblow
#define MH_X25_COLOR_LIGHT_BLUE 27   // Light blue
#define MH_X25_COLOR_YELLOW_GREEN 32 // Yellow green
#define MH_X25_COLOR_RED 37          // Red
#define MH_X25_COLOR_DARK_BLUE 42    // Dark blue
#define MH_X25_COLOR_RAINBOW_CW 160  // Rainbow clockwise (mid-range)
#define MH_X25_COLOR_RAINBOW_CCW 224 // Rainbow counter-clockwise (mid-range)

/* Shutter/Strobe Values (Channel 4) */
#define MH_X25_SHUTTER_BLACKOUT 0      // Blackout
#define MH_X25_SHUTTER_OPEN 7          // Open
#define MH_X25_SHUTTER_STROBE_SLOW 50  // Strobe slow
#define MH_X25_SHUTTER_STROBE_MED 130  // Strobe medium
#define MH_X25_SHUTTER_STROBE_FAST 200 // Strobe fast

/* Gobo Wheel Values (Channel 5) */
#define MH_X25_GOBO_OPEN 0          // Open (no gobo)
#define MH_X25_GOBO_2 12            // Gobo 2
#define MH_X25_GOBO_3 20            // Gobo 3
#define MH_X25_GOBO_4 28            // Gobo 4
#define MH_X25_GOBO_5 36            // Gobo 5
#define MH_X25_GOBO_6 44            // Gobo 6
#define MH_X25_GOBO_7 52            // Gobo 7
#define MH_X25_GOBO_8 60            // Gobo 8
#define MH_X25_GOBO_RAINBOW_CW 160  // Gobo rainbow clockwise
#define MH_X25_GOBO_RAINBOW_CCW 224 // Gobo rainbow counter-clockwise

/* Gobo Rotation Values (Channel 6) */
#define MH_X25_GOBO_ROT_STOP 32      // Fixed position
#define MH_X25_GOBO_ROT_CW_SLOW 80   // Rotate clockwise slow
#define MH_X25_GOBO_ROT_CW_FAST 130  // Rotate clockwise fast
#define MH_X25_GOBO_ROT_CCW_SLOW 180 // Rotate counter-clockwise slow
#define MH_X25_GOBO_ROT_CCW_FAST 220 // Rotate counter-clockwise fast

    /**
     * @brief MH X25 Device Configuration
     */
    typedef struct
    {
        dmx_handle_t dmx_handle; ///< DMX driver handle
        uint16_t start_channel;  ///< DMX start channel (1-507, allows for 6 channels)
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
     * @brief Set pan (horizontal rotation) position
     *
     * @param handle Device handle
     * @param pan Pan value (0-255, maps to 0° to max pan range)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_pan(mh_x25_handle_t handle, uint8_t pan);

    /**
     * @brief Set tilt (vertical inclination) position
     *
     * @param handle Device handle
     * @param tilt Tilt value (0-255, maps to 0° to max tilt range)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_tilt(mh_x25_handle_t handle, uint8_t tilt);

    /**
     * @brief Set both pan and tilt together
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
     * @brief Set color wheel position
     *
     * @param handle Device handle
     * @param color Color value (see MH_X25_COLOR_* constants)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_color(mh_x25_handle_t handle, uint8_t color);

    /**
     * @brief Set shutter/strobe
     *
     * @param handle Device handle
     * @param shutter Shutter value (see MH_X25_SHUTTER_* constants)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_shutter(mh_x25_handle_t handle, uint8_t shutter);

    /**
     * @brief Set gobo pattern
     *
     * @param handle Device handle
     * @param gobo Gobo value (see MH_X25_GOBO_* constants)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_gobo(mh_x25_handle_t handle, uint8_t gobo);

    /**
     * @brief Set gobo rotation
     *
     * @param handle Device handle
     * @param rotation Rotation value (see MH_X25_GOBO_ROT_* constants)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_gobo_rotation(mh_x25_handle_t handle, uint8_t rotation);

    /**
     * @brief Set all channels at once
     *
     * @param handle Device handle
     * @param pan Pan value (0-255)
     * @param tilt Tilt value (0-255)
     * @param color Color value (0-255)
     * @param shutter Shutter value (0-255)
     * @param gobo Gobo value (0-255)
     * @param gobo_rot Gobo rotation value (0-255)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_set_all(mh_x25_handle_t handle, uint8_t pan, uint8_t tilt,
                             uint8_t color, uint8_t shutter, uint8_t gobo, uint8_t gobo_rot);

    /**
     * @brief Turn off the light (blackout)
     *
     * @param handle Device handle
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t mh_x25_off(mh_x25_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // MH_X25_H
