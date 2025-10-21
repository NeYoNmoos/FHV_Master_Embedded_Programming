/* Simple LED & Button Test for Clownfish ESP32-C3
 * - 25 LEDs on GPIO 8
 * - Button 1 (Increase) on GPIO 9 (BOOT button)
 * - Button 2 (Decrease) on GPIO 2
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "test";

// GPIO Configuration
#define LED_GPIO 8
#define BUTTON_INCREASE_GPIO 9
#define BUTTON_DECREASE_GPIO 2
#define LED_COUNT 25

// LED strip handle
static led_strip_handle_t led_strip;

// LED functions
void led_init(void)
{
    ESP_LOGI(TAG, "Initializing %d LEDs on GPIO %d", LED_COUNT, LED_GPIO);

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
}

void led_set_all(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        led_strip_set_pixel(led_strip, i, r, g, b);
    }
    led_strip_refresh(led_strip);
}

void led_clear(void)
{
    led_strip_clear(led_strip);
}

// Button functions
void button_init(void)
{
    ESP_LOGI(TAG, "Initializing buttons: Increase=%d, Decrease=%d",
             BUTTON_INCREASE_GPIO, BUTTON_DECREASE_GPIO);

    // Configure increase button (GPIO 9 - BOOT)
    gpio_config_t btn_inc = {
        .pin_bit_mask = (1ULL << BUTTON_INCREASE_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_inc);

    // Configure decrease button (GPIO 2)
    gpio_config_t btn_dec = {
        .pin_bit_mask = (1ULL << BUTTON_DECREASE_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_dec);
}

bool button_increase_pressed(void)
{
    return (gpio_get_level(BUTTON_INCREASE_GPIO) == 0);
}

bool button_decrease_pressed(void)
{
    return (gpio_get_level(BUTTON_DECREASE_GPIO) == 0);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== LED & Button Test ===");

    // Initialize hardware
    led_init();
    button_init();

    ESP_LOGI(TAG, "Ready! Press buttons to test.");
    ESP_LOGI(TAG, "- BOOT button (GPIO9) -> All LEDs GREEN");
    ESP_LOGI(TAG, "- GPIO2 button -> All LEDs RED");

    // Simple test loop
    while (1)
    {
        if (button_increase_pressed())
        {
            ESP_LOGI(TAG, "Increase button pressed!");
            led_set_all(0, 50, 0); // Green
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else if (button_decrease_pressed())
        {
            ESP_LOGI(TAG, "Decrease button pressed!");
            led_set_all(50, 0, 0); // Red
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else
        {
            led_clear(); // LEDs off
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
