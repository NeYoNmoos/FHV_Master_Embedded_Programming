/**
 * @file espnow_handler.h
 * @author Matthias Hefel
 * @date 2026
 * @brief ESP-NOW communication handler with dynamic peer discovery
 */

#ifndef ESPNOW_HANDLER_H
#define ESPNOW_HANDLER_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_now.h"
#include "esp_err.h"

// Event bits for paddle hits
#define PADDLE_TOP_HIT BIT0
#define PADDLE_BOTTOM_HIT BIT1

// Broadcast MAC address for ESP-NOW
#define ESPNOW_BROADCAST_MAC ((const uint8_t[]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF})

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Message type enumeration
     */
    typedef enum
    {
        MSG_HELLO = 0,        // Client registration request
        MSG_PADDLE_INPUT = 1, // Paddle input data
        MSG_GAME_SCORE = 2,   // Game score broadcast
        MSG_SERVER_ASSIGN = 3 // Server player ID assignment
    } msg_type_t;

    /**
     * @brief Hello message from client (registration request)
     */
    typedef struct
    {
        uint8_t type; // MSG_HELLO
    } hello_t;

    /**
     * @brief Server assignment response
     */
    typedef struct
    {
        uint8_t type;      // MSG_SERVER_ASSIGN
        uint8_t player_id; // Assigned player ID (1 or 2)
        uint8_t status;    // 0=accepted, 1=game_full, 2=already_registered
    } server_assign_t;

    /**
     * @brief Input event data structure from paddle controllers
     */
    typedef struct
    {
        uint8_t type; // MSG_PADDLE_INPUT
        uint8_t id;   // Player ID (assigned by server)
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
     * @param recv_info Reception info containing source MAC address
     * @param data Received data buffer
     * @param len Length of received data
     */
    void on_receive(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);

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

    /**
     * @brief Get number of registered players
     * @return Number of registered players (0-2)
     */
    uint8_t espnow_get_num_players(void);

    /**
     * @brief Get player ID from MAC address
     * @param mac_addr MAC address to lookup
     * @return Player ID (1 or 2), or 0 if not found
     */
    uint8_t espnow_get_player_id(const uint8_t *mac_addr);

    /**
     * @brief Broadcast game score to all connected players
     * @param score Pointer to game score structure
     * @param size Size of the score structure in bytes
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t espnow_broadcast_score(const void *score, size_t size);

#ifdef __cplusplus
}
#endif

#endif // ESPNOW_HANDLER_H
