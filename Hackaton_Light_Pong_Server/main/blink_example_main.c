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

/**
 * @brief Circle movement demo
 *
 * This function makes the light move in a circular pattern.
 *
 * How it works:
 * - Pan controls horizontal rotation (left-right)
 * - Tilt controls vertical movement (up-down)
 * - We use sine and cosine to create circular motion
 * - The circle repeats continuously
 */
static void demo_circle_movement(void)
{
    ESP_LOGI(TAG, "=== Circle Movement Demo ===");
    ESP_LOGI(TAG, "The light will move in a circular pattern");

    const int steps = 100;      // Number of steps in the circle
    const int delay_ms = 30;    // Delay between each step
    const float radius = 50.0f; // Size of the circle (0-127)

    // Set shutter open and a nice color
    mh_x25_set_shutter(light_handle, MH_X25_SHUTTER_OPEN);
    mh_x25_set_color(light_handle, MH_X25_COLOR_GREEN);
    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);

    // Center positions (middle of the movement range)
    const uint8_t center_pan = 128;  // Center horizontal
    const uint8_t center_tilt = 128; // Center vertical

    ESP_LOGI(TAG, "Starting circular motion...");

    for (int i = 0; i < steps; i++)
    {
        // Calculate angle for this step (0 to 2*PI)
        float angle = (2.0f * M_PI * i) / steps;

        // Calculate pan and tilt using sine and cosine
        // This creates a perfect circle
        uint8_t pan = center_pan + (int)(radius * cosf(angle));
        uint8_t tilt = center_tilt + (int)(radius * sinf(angle));

        // Set the new position
        mh_x25_set_position(light_handle, pan, tilt);

        // Small delay for smooth movement
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    ESP_LOGI(TAG, "Circle complete!");
}

/**
 * @brief Color wheel demo while moving
 */
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

    mh_x25_set_shutter(light_handle, MH_X25_SHUTTER_OPEN);

    mh_x25_set_color(light_handle, MH_X25_COLOR_PINK);
    mh_x25_set_position(light_handle, 128, 128);
    vTaskDelay(pdMS_TO_TICKS(2000));
    mh_x25_set_color(light_handle, MH_X25_COLOR_DARK_BLUE);
    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4);
    mh_x25_set_gobo_rotation(light_handle, MH_X25_GOBO_ROT_CCW_FAST);
    mh_x25_set_position(light_handle, 128, 0);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // mh_x25_set_position(light_handle, 0, 0);
    // vTaskDelay(pdMS_TO_TICKS(500));
    // for (int color_idx = 0; color_idx < num_colors; color_idx++)
    // {
    //     ESP_LOGI(TAG, "Color %d", color_idx + 1);
    //     mh_x25_set_color(light_handle, colors[color_idx]);

    //     for (int i = 0; i < steps_per_color; i++)
    //     {
    //         float angle = (2.0f * M_PI * i) / steps_per_color;
    //         uint8_t pan = 128 + (int)(radius * cosf(angle));
    //         uint8_t tilt = 128 + (int)(radius * sinf(angle));

    //         mh_x25_set_position(light_handle, pan, tilt);
    //         vTaskDelay(pdMS_TO_TICKS(delay_ms));
    //     }
    // }
}

/**
 * @brief Figure-8 pattern demo
 */
static void demo_figure_eight(void)
{
    ESP_LOGI(TAG, "=== Figure-8 Pattern ===");

    const int steps = 200;
    const int delay_ms = 20;

    mh_x25_set_shutter(light_handle, MH_X25_SHUTTER_OPEN);
    mh_x25_set_color(light_handle, MH_X25_COLOR_LIGHT_BLUE);

    for (int i = 0; i < steps; i++)
    {
        float t = (2.0f * M_PI * i) / steps;

        // Lissajous curve (figure-8)
        uint8_t pan = 128 + (int)(50.0f * sinf(t));
        uint8_t tilt = 128 + (int)(50.0f * sinf(2.0f * t));

        mh_x25_set_position(light_handle, pan, tilt);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

// Task 1: ESP-NOW Receiver
void espnow_receiver_task(void *pvParameters)
{
    ESP_LOGI(TAG, "ESP-NOW receiver task started");

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
