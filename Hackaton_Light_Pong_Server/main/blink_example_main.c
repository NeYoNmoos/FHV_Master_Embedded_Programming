/* DMX512 MH X25 Control Example

   This example demonstrates how to control the MH X25 LED moving head light
   via DMX512 protocol over RS-485.
*/
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "dmx.h"
#include "mh_x25.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char *TAG = "dmx_example";

/* DMX Configuration for Clownfish ESP32-C3
 * IMPORTANT: Do NOT use GPIO18/19 - those are USB D-/D+ pins!
 * Using GPIO18/19 will disable USB and prevent reprogramming!
 */
#define DMX_TX_PIN GPIO_NUM_21    // 21 Wichtig!
#define DMX_RX_PIN GPIO_NUM_20    // 20 Wichtig!
#define DMX_ENABLE_PIN GPIO_NUM_9 // 9 Wichtig!

/* MH X25 DMX Configuration */
#define MH_X25_START_CHANNEL 1 // DMX start address (channels 1-6)

static dmx_handle_t dmx_handle = NULL;
static mh_x25_handle_t light_handle = NULL;

static void demo_circle_with_colors(void)
{
    ESP_LOGI(TAG, "=== Circle with Color Changes ===");

    const uint8_t colors[] = {
        MH_X25_COLOR_RED,
        MH_X25_COLOR_GREEN,
        MH_X25_COLOR_LIGHT_BLUE,
        MH_X25_COLOR_YELLOW,
        MH_X25_COLOR_PINK};
    const int num_colors = sizeof(colors) / sizeof(colors[0]);

    const int steps_per_color = 50;
    const int delay_ms = 40;
    const float radius = 60.0f;

    const int corner_values[4][2] = {{128 + 20, 128 + 60}, {128 - 20, 128 + 60}, {128 - 20, 128 - 60}, {128 + 20, 128 - 60}};

    const int border_values[4][2] = {{128 + 45, 128 + 50}, {128 + 45, 128 - 50}, {128, 128 + 60}, {128, 128 - 60}};

    mh_x25_set_shutter(light_handle, MH_X25_SHUTTER_OPEN);

    mh_x25_set_color(light_handle, MH_X25_COLOR_PINK);
    // mh_x25_set_position(light_handle, 180, 10);
    // vTaskDelay(pdMS_TO_TICKS(2000));
    mh_x25_set_color(light_handle, MH_X25_COLOR_DARK_BLUE);
    // mh_x25_set_gobo(light_handle, MH_X25_GOBO_3);
    // mh_x25_set_shutter(light_handle, 150);
    mh_x25_set_gobo_rotation(light_handle, MH_X25_GOBO_ROT_CCW_FAST);
    // mh_x25_set_position(light_handle, 128, 128);
    // vTaskDelay(pdMS_TO_TICKS(2000));

    mh_x25_set_position(light_handle, border_values[0][0], border_values[0][1]);
    vTaskDelay(pdMS_TO_TICKS(2000));

    mh_x25_set_position(light_handle, border_values[1][0], border_values[1][1]);
    vTaskDelay(pdMS_TO_TICKS(2000));

    mh_x25_set_position(light_handle, border_values[2][0], border_values[2][1]);
    vTaskDelay(pdMS_TO_TICKS(4000));

    mh_x25_set_position(light_handle, border_values[3][0], border_values[3][1]);
    vTaskDelay(pdMS_TO_TICKS(4000));

    mh_x25_set_position(light_handle, corner_values[0][0], corner_values[0][1]);
    vTaskDelay(pdMS_TO_TICKS(2000));

    mh_x25_set_position(light_handle, corner_values[1][0], corner_values[1][1]);
    vTaskDelay(pdMS_TO_TICKS(2000));

    mh_x25_set_position(light_handle, corner_values[2][0], corner_values[2][1]);
    vTaskDelay(pdMS_TO_TICKS(2000));

    mh_x25_set_position(light_handle, corner_values[3][0], corner_values[3][1]);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void on_receive(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    ESP_LOGI(TAG, "Received from MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1],
             mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
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
    printf("ESP32-C3 MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3],
           mac[4], mac[5]);

    while (1)
    {
        // Your ESP-NOW receiving code here
        // This runs independently

        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay
    }
}

// Task 2: DMX Controller
void dmx_controller_task(void *pvParameters)
{
    ESP_LOGI(TAG, "DMX controller task started");

    // Main demo loop
    int loop_count = 0;
    while (1)
    {
        loop_count++;
        ESP_LOGI(TAG, "\n\n========== DEMO CYCLE #%d ==========\n", loop_count);
        // Circle with color changes
        ESP_LOGI(TAG, ">>> Starting Demo");
        demo_circle_with_colors();
        ESP_LOGI(TAG, "<<< Demo  Complete");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "DMX512 MH X25 Control Example");
    ESP_LOGI(TAG, "==============================");

    // Initialize DMX
    dmx_config_t dmx_config = {
        .tx_pin = DMX_TX_PIN,
        .rx_pin = DMX_RX_PIN,
        .enable_pin = DMX_ENABLE_PIN,
        .uart_num = UART_NUM_1,
        .universe_size = 512 // Full DMX universe
    };

    esp_err_t ret = dmx_init(&dmx_config, &dmx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize DMX: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "DMX initialized successfully");

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
    ESP_LOGI(TAG, "DMX transmission started at 44Hz");

    // Wait a moment for DMX to stabilize
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Starting movement demos...");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "MOVEMENT CONTROL EXPLAINED:");
    ESP_LOGI(TAG, "- Pan (Channel 1): 0-255 controls horizontal rotation");
    ESP_LOGI(TAG, "- Tilt (Channel 2): 0-255 controls vertical inclination");
    ESP_LOGI(TAG, "- 128 is the center position for both");
    ESP_LOGI(TAG, "- Use mh_x25_set_position(handle, pan, tilt) to move");
    ESP_LOGI(TAG, "");

    xTaskCreate(
        espnow_receiver_task, // Task function
        "espnow_rx",          // Task name
        4096,                 // Stack size (bytes)
        NULL,                 // Parameters
        5,                    // Priority (higher = more important)
        NULL                  // Task handle (optional)
    );

    // Create DMX controller task
    xTaskCreate(
        dmx_controller_task, // Task function
        "dmx_ctrl",          // Task name
        4096,                // Stack size (bytes)
        NULL,                // Parameters
        5,                   // Priority
        NULL                 // Task handle
    );

    ESP_LOGI(TAG, "Both tasks created and running!");
}
