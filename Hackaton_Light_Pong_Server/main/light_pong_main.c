/**
 * @file light_pong_main.c
 * @author Matthias Hefel
 * @date 2026
 * @brief Light Pong Game - Main Application
 *
 * DMX512-based Light Pong game using MH-X25 LED moving head.
 * Features ESP-NOW wireless paddles, dynamic peer discovery, and fireball effects.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "dmx_driver.h"
#include "mh_x25_driver.h"
#include "config/hardware_config.h"
#include "config/game_config.h"
#include "espnow_handler.h"
#include "game/game_controller.h"
#include "game/game_types.h"

static const char *TAG = "main";

static EventGroupHandle_t paddle_events;
static volatile int current_side = SIDE_TOP;
static volatile uint8_t last_btn_left_pressed = 0;
static volatile uint8_t last_btn_right_pressed = 0;

static dmx_handle_t dmx_handle = NULL;
static mh_x25_handle_t light_handle = NULL;

static game_score_t game_score = {0, 0};

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing Light Pong Game");

    // Create event group for paddle communication
    paddle_events = xEventGroupCreate();
    if (paddle_events == NULL)
    {
        ESP_LOGE(TAG, "Failed to create event group");
        return;
    }

    // Initialize DMX
    dmx_config_t dmx_config = {
        .tx_pin = DMX_TX_PIN,
        .rx_pin = DMX_RX_PIN,
        .enable_pin = DMX_ENABLE_PIN,
        .uart_num = UART_NUM_1,
        .universe_size = 512};

    esp_err_t ret = dmx_init(&dmx_config, &dmx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize DMX: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize MH X25 light
    mh_x25_config_t light_config = {
        .dmx_handle = dmx_handle,
        .start_channel = MH_X25_START_CHANNEL};

    ret = mh_x25_init(&light_config, &light_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize MH X25: %s", esp_err_to_name(ret));
        dmx_deinit(dmx_handle);
        return;
    }
    ESP_LOGI(TAG, "MH X25 initialized at DMX address %d", MH_X25_START_CHANNEL);

    // Start continuous DMX transmission
    ret = dmx_start_transmission(dmx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start DMX transmission: %s", esp_err_to_name(ret));
        mh_x25_deinit(light_handle);
        dmx_deinit(dmx_handle);
        return;
    }

    // Wait for DMX to stabilize
    vTaskDelay(pdMS_TO_TICKS(500));

    // Set context for communication module (inject dependencies)
    espnow_set_context(paddle_events, (uint8_t *)&last_btn_left_pressed, (uint8_t *)&last_btn_right_pressed);

    // Set context for game controller (inject dependencies)
    game_controller_set_context(light_handle, paddle_events, &current_side,
                                (uint8_t *)&last_btn_left_pressed, (uint8_t *)&last_btn_right_pressed,
                                &game_score);

    xTaskCreate(
        espnow_receiver_task,
        "espnow_rx",
        4096,
        NULL,
        5,
        NULL);

    xTaskCreate(
        dmx_controller_task,
        "dmx_ctrl",
        4096,
        NULL,
        5,
        NULL);

    ESP_LOGI(TAG, "System initialized successfully");

    // Main loop - keep running
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
