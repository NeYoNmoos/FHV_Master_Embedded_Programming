/**
 * @file espnow_handler.h
 * @brief ESP-NOW communication handler
 */

#ifndef ESPNOW_HANDLER_H
#define ESPNOW_HANDLER_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Input event data structure from paddle controllers
     */
    typedef struct
    {
        uint8_t id;
        uint8_t btn_right_pressed;
        uint8_t btn_left_pressed;
        float ax, ay, az;
        float gx, gy, gz;
    } input_event_t;

    /**
     * @brief Initialize ESP-NOW and start receiver task
     *
     * @param paddle_events Event group handle for paddle hit synchronization
     * @param btn_left Pointer to store left button state
     * @param btn_right Pointer to store right button state
     */
    void espnow_receiver_task(void *pvParameters);

    /**
     * @brief Add ESP-NOW peer
     *
     * @param mac MAC address of peer to add
     */
    void add_peer(const uint8_t mac[6]);

    /**
     * @brief ESP-NOW receive callback
     *
     * @param mac_addr MAC address of sender
     * @param data Received data buffer
     * @param len Length of received data
     */
    void on_receive(const uint8_t *mac_addr, const uint8_t *data, int len);

    /**
     * @brief Get last button state for left paddle
     */
    uint8_t espnow_get_left_button(void);

    /**
     * @brief Get last button state for right paddle
     */
    uint8_t espnow_get_right_button(void);

    /**
     * @brief Set event group and button pointers for communication
     */
    void espnow_set_context(EventGroupHandle_t events, volatile uint8_t *btn_left, volatile uint8_t *btn_right);

#ifdef __cplusplus
}
#endif

#endif // ESPNOW_HANDLER_H
