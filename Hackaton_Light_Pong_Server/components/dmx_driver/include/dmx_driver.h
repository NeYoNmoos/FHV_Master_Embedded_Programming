/**
 * @file dmx_driver.h
 * @author Matthias Hefel
 * @date 2026
 * @brief DMX512 Protocol Driver for ESP32-C3
 *
 * This driver implements DMX512 protocol over RS-485 using UART.
 * DMX512 is a unidirectional protocol commonly used for lighting control.
 */

#ifndef DMX_DRIVER_H
#define DMX_DRIVER_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* DMX512 Protocol Constants */
#define DMX_UNIVERSE_SIZE 512      // Maximum DMX channels
#define DMX_BREAK_US 92            // Break time in microseconds (88-1000us)
#define DMX_MAB_US 12              // Mark After Break (8-1000us)
#define DMX_PACKET_TIMEOUT_MS 1000 // Timeout for packet transmission

/* Default GPIO Configuration for Clownfish ESP32-C3 */
/* Adjust these based on your actual board layout */
#define DMX_DEFAULT_TX_PIN GPIO_NUM_21  // UART TX pin
#define DMX_DEFAULT_RX_PIN GPIO_NUM_20  // UART RX pin (not used in DMX TX mode)
#define DMX_DEFAULT_RTS_PIN GPIO_NUM_19 // RS-485 DE/RE control pin

/* UART Configuration for DMX512 */
#define DMX_UART_NUM UART_NUM_1
#define DMX_BAUD_RATE 250000 // DMX512 standard baud rate
#define DMX_DATA_BITS UART_DATA_8_BITS
#define DMX_PARITY UART_PARITY_DISABLE
#define DMX_STOP_BITS UART_STOP_BITS_2

    /**
     * @brief DMX Configuration Structure
     */
    typedef struct
    {
        gpio_num_t tx_pin;      ///< UART TX pin
        gpio_num_t rx_pin;      ///< UART RX pin (optional, not used in TX-only mode)
        gpio_num_t enable_pin;  ///< RS-485 DE/RE control pin
        uart_port_t uart_num;   ///< UART port number
        uint16_t universe_size; ///< Number of DMX channels (1-512)
    } dmx_config_t;

    /**
     * @brief DMX driver handle
     */
    typedef void *dmx_handle_t;

    /**
     * @brief Initialize DMX driver
     *
     * @param config Pointer to DMX configuration structure
     * @param out_handle Pointer to store the DMX handle
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     *      - ESP_ERR_NO_MEM: Out of memory
     *      - ESP_FAIL: UART configuration failed
     */
    esp_err_t dmx_init(const dmx_config_t *config, dmx_handle_t *out_handle);

    /**
     * @brief Deinitialize DMX driver
     *
     * @param handle DMX handle
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid handle
     */
    esp_err_t dmx_deinit(dmx_handle_t handle);

    /**
     * @brief Set DMX channel value
     *
     * @param handle DMX handle
     * @param channel Channel number (1-512)
     * @param value Channel value (0-255)
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t dmx_set_channel(dmx_handle_t handle, uint16_t channel, uint8_t value);

    /**
     * @brief Set multiple DMX channels
     *
     * @param handle DMX handle
     * @param start_channel Starting channel number (1-512)
     * @param data Pointer to data array
     * @param length Number of channels to set
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t dmx_set_channels(dmx_handle_t handle, uint16_t start_channel,
                               const uint8_t *data, uint16_t length);

    /**
     * @brief Get DMX channel value
     *
     * @param handle DMX handle
     * @param channel Channel number (1-512)
     * @param value Pointer to store the channel value
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid arguments
     */
    esp_err_t dmx_get_channel(dmx_handle_t handle, uint16_t channel, uint8_t *value);

    /**
     * @brief Transmit DMX packet
     *
     * Sends a complete DMX512 packet including break, MAB, and data.
     * This function should be called periodically (typically 44Hz or 25-44ms interval).
     *
     * @param handle DMX handle
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid handle
     *      - ESP_FAIL: Transmission failed
     */
    esp_err_t dmx_transmit(dmx_handle_t handle);

    /**
     * @brief Start continuous DMX transmission
     *
     * Starts a task that continuously transmits DMX packets at ~44Hz.
     *
     * @param handle DMX handle
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid handle
     *      - ESP_ERR_INVALID_STATE: Already running
     */
    esp_err_t dmx_start_transmission(dmx_handle_t handle);

    /**
     * @brief Stop continuous DMX transmission
     *
     * @param handle DMX handle
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid handle
     */
    esp_err_t dmx_stop_transmission(dmx_handle_t handle);

    /**
     * @brief Clear all DMX channels (set to 0)
     *
     * @param handle DMX handle
     * @return
     *      - ESP_OK: Success
     *      - ESP_ERR_INVALID_ARG: Invalid handle
     */
    esp_err_t dmx_clear_all(dmx_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // DMX_DRIVER_H
