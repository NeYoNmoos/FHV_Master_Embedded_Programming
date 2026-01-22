/**
 * @file espnow_handler.c
 * @brief ESP-NOW communication handler implementation
 */

#include "espnow_handler.h"
#include "../config/game_config.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <string.h>

static const char *TAG = "espnow_handler";

// Maximum number of players
#define MAX_PLAYERS 2

// Broadcast MAC address
static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Context for communication with game controller
static EventGroupHandle_t paddle_events = NULL;
static volatile uint8_t *last_btn_left_pressed = NULL;
static volatile uint8_t *last_btn_right_pressed = NULL;

// Dynamic player registry
static uint8_t player_macs[MAX_PLAYERS][6];
static uint8_t num_players = 0;

void espnow_set_context(EventGroupHandle_t events, volatile uint8_t *btn_left, volatile uint8_t *btn_right)
{
    paddle_events = events;
    last_btn_left_pressed = btn_left;
    last_btn_right_pressed = btn_right;
}

uint8_t espnow_get_num_players(void)
{
    return num_players;
}

uint8_t espnow_get_player_id(const uint8_t *mac_addr)
{
    for (uint8_t i = 0; i < num_players; i++)
    {
        if (memcmp(player_macs[i], mac_addr, 6) == 0)
        {
            return i + 1; // Player IDs are 1-based
        }
    }
    return 0; // Not found
}

static void handle_hello_message(const uint8_t *mac_addr)
{
    // Check if player already registered
    uint8_t existing_id = espnow_get_player_id(mac_addr);
    if (existing_id > 0)
    {
        ESP_LOGI(TAG, "Player already registered as ID %d", existing_id);

        // Send assignment confirmation via broadcast (client may not have added us as peer yet)
        server_assign_t assign = {
            .type = MSG_SERVER_ASSIGN,
            .player_id = existing_id,
            .status = 2 // Already registered
        };
        esp_now_send(BROADCAST_MAC, (uint8_t *)&assign, sizeof(assign)); // Broadcast
        return;
    }

    // Check if game is full
    if (num_players >= MAX_PLAYERS)
    {
        ESP_LOGW(TAG, "Game full, rejecting new player");

        server_assign_t assign = {
            .type = MSG_SERVER_ASSIGN,
            .player_id = 0,
            .status = 1 // Game full
        };
        esp_now_send(BROADCAST_MAC, (uint8_t *)&assign, sizeof(assign)); // Broadcast
        return;
    }

    // Register new player
    memcpy(player_macs[num_players], mac_addr, 6);
    num_players++;
    uint8_t assigned_id = num_players;

    // Add as ESP-NOW peer
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, mac_addr, 6);
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.channel = 0;
    peer.encrypt = false;
    esp_err_t ret = esp_now_add_peer(&peer);

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "✅ Player %d registered: %02X:%02X:%02X:%02X:%02X:%02X",
                 assigned_id,
                 mac_addr[0], mac_addr[1], mac_addr[2],
                 mac_addr[3], mac_addr[4], mac_addr[5]);

        // Send assignment via broadcast (client hasn't added us as peer yet)
        server_assign_t assign = {
            .type = MSG_SERVER_ASSIGN,
            .player_id = assigned_id,
            .status = 0 // Accepted
        };
        esp_now_send(BROADCAST_MAC, (uint8_t *)&assign, sizeof(assign)); // Broadcast
        ESP_LOGI(TAG, "📤 Assignment broadcast sent to Player %d", assigned_id);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to add peer: %s", esp_err_to_name(ret));
        num_players--; // Rollback
    }
}

static void handle_paddle_input(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    if (len < sizeof(input_event_t))
    {
        ESP_LOGW(TAG, "Invalid paddle input size: %d", len);
        return;
    }

    input_event_t *m = (input_event_t *)data;
    uint8_t player_id = espnow_get_player_id(mac_addr);

    if (player_id == 0)
    {
        ESP_LOGW(TAG, "Received input from unregistered player");
        return;
    }

    // Handle based on player ID
    if (player_id == 1)
    {
        if (last_btn_left_pressed != NULL)
        {
            *last_btn_left_pressed = m->btn_right_pressed; // switch on purpose
        }
        ESP_LOGI(TAG, "LEFT PADDLE (Player 1) HIT! Button: %d", m->btn_right_pressed);

        if (paddle_events != NULL)
        {
            xEventGroupSetBits(paddle_events, PADDLE_TOP_HIT);
        }
    }
    else if (player_id == 2)
    {
        if (last_btn_right_pressed != NULL)
        {
            *last_btn_right_pressed = m->btn_left_pressed; // switch on purpose
        }
        ESP_LOGI(TAG, "RIGHT PADDLE (Player 2) HIT! Button: %d", m->btn_left_pressed);

        if (paddle_events != NULL)
        {
            xEventGroupSetBits(paddle_events, PADDLE_BOTTOM_HIT);
        }
    }
}

void on_receive(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    if (len < 1)
        return; // Safety check

    // Extract source MAC address from recv_info
    const uint8_t *mac_addr = recv_info->src_addr;

    // Get message type from first byte
    uint8_t msg_type = data[0];

    switch (msg_type)
    {
    case MSG_HELLO:
        if (len == sizeof(hello_t))
        {
            ESP_LOGI(TAG, "📡 Received HELLO from %02X:%02X:%02X:%02X:%02X:%02X (len=%d)",
                     mac_addr[0], mac_addr[1], mac_addr[2],
                     mac_addr[3], mac_addr[4], mac_addr[5], len);
            handle_hello_message(mac_addr);
        }
        else
        {
            ESP_LOGW(TAG, "Invalid HELLO message size: %d", len);
        }
        break;

    case MSG_PADDLE_INPUT:
        handle_paddle_input(mac_addr, data, len);
        break;

    default:
        ESP_LOGW(TAG, "Unknown message type: %d", msg_type);
        break;
    }
}

void add_peer(const uint8_t mac[6])
{
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, mac, 6);
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
}

// Task 1: ESP-NOW Receiver
void espnow_receiver_task(void *pvParameters)
{
    ESP_LOGI(TAG, "ESP-NOW receiver task started");

    // esp now init
    // NVS init
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    // WiFi Station Mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // Set channel 1 for ESP-NOW broadcasts

    // ESP-NOW init
    esp_now_init();
    esp_now_register_recv_cb((esp_now_recv_cb_t)on_receive);

    // Add broadcast address as peer to enable broadcasting
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t broadcast_peer = {0};
    memcpy(broadcast_peer.peer_addr, broadcast_mac, 6);
    broadcast_peer.ifidx = ESP_IF_WIFI_STA;
    broadcast_peer.channel = 0;
    broadcast_peer.encrypt = false;
    esp_err_t ret = esp_now_add_peer(&broadcast_peer);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "✅ Broadcast peer added");
    }
    else
    {
        ESP_LOGE(TAG, "❌ Failed to add broadcast peer: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Master bereit...");

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    printf("ESP32-C3 WIFI MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3],
           mac[4], mac[5]);

    uint8_t mac2[6];
    esp_read_mac(mac2, ESP_MAC_WIFI_STA);
    printf("ESP32-C3 MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac2[0], mac2[1], mac2[2], mac2[3],
           mac2[4], mac2[5]);

    ESP_LOGI(TAG, "🎮 Server ready! Waiting for players to connect...");
    ESP_LOGI(TAG, "Players should send HELLO message to register");

    while (1)
    {
        // Your ESP-NOW receiving code here
        // This runs independently

        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay
    }
}
