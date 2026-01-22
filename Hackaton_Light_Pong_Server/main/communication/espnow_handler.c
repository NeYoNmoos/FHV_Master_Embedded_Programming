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

// Context for communication with game controller
static EventGroupHandle_t paddle_events = NULL;
static volatile uint8_t *last_btn_left_pressed = NULL;
static volatile uint8_t *last_btn_right_pressed = NULL;

void espnow_set_context(EventGroupHandle_t events, volatile uint8_t *btn_left, volatile uint8_t *btn_right)
{
    paddle_events = events;
    last_btn_left_pressed = btn_left;
    last_btn_right_pressed = btn_right;
}

void on_receive(const uint8_t *mac_addr, const uint8_t *data, int len)
{

    ESP_LOGI(TAG, "ESP-NOW data received, length: %d", len);
    if (len < 1)
        return; // safety check

    // Cast data to input_event_t
    input_event_t *m = (input_event_t *)data;

    // Check which paddle sent the signal based on ID
    if (m->id == 1)
    {
        if (last_btn_left_pressed != NULL)
        {
            *last_btn_left_pressed = m->btn_right_pressed; // switch on npurpouse
        }
        ESP_LOGI(TAG, "LEFT PADDLE (ID=1) HIT detected! Button pressed: %d", m->btn_right_pressed);

        if (paddle_events != NULL)
        {
            xEventGroupSetBits(paddle_events, PADDLE_TOP_HIT);
        }
    }
    else if (m->id == 2)
    {
        if (last_btn_right_pressed != NULL)
        {
            *last_btn_right_pressed = m->btn_left_pressed; // switch on npurpouse
        }
        ESP_LOGI(TAG, "RIGHT PADDLE (ID=2) HIT detected! Button pressed: %d", m->btn_left_pressed);

        if (paddle_events != NULL)
        {
            xEventGroupSetBits(paddle_events, PADDLE_BOTTOM_HIT);
        }
    }
    else
    {
        ESP_LOGW(TAG, "Unknown paddle ID: %d", m->id);
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

    // ESP-NOW init
    esp_now_init();
    esp_now_register_recv_cb((esp_now_recv_cb_t)on_receive);

    ESP_LOGI(TAG, "Master bereit...");

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    printf("ESP32-C3 WIFI MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3],
           mac[4], mac[5]);

    uint8_t mac2[6];
    esp_read_mac(mac2, ESP_MAC_WIFI_STA);
    printf("ESP32-C3 MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac2[0], mac2[1], mac2[2], mac2[3],
           mac2[4], mac2[5]);

    // Add peers AFTER ESP-NOW is initialized
    uint8_t player1_esp[] = PLAYER1_MAC;
    uint8_t player2_esp[] = PLAYER2_MAC;

    add_peer(player1_esp);
    add_peer(player2_esp);

    ESP_LOGI(TAG, "Added ESP-NOW peers");

    while (1)
    {
        // Your ESP-NOW receiving code here
        // This runs independently

        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay
    }
}
