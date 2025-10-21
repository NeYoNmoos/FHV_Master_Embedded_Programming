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
#include "esp_mac.h"

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

void display_minutes(int minutes);
void display_seconds(int seconds);

void display_time(int seconds)
{
    int minutes = seconds / 60;
    int secs = seconds % 60;

    display_minutes(minutes);
    display_seconds(secs);

    ESP_LOGI(TAG, "Time: %02d:%02d", minutes, secs);
}

void display_minutes(int minutes)
{

    // display minutes as binary on led 0 to 4
    for (int i = 0; i < 5; i++)
    {
        if (minutes & (1 << i))
        {
            led_strip_set_pixel(led_strip, i, 0, 25, 25);
        }
        else
        {
            led_strip_set_pixel(led_strip, i, 0, 0, 0); // Off
        }
    }
    led_strip_refresh(led_strip);
}

void display_seconds(int seconds)
{
    // Display seconds as binary on LED 5 to 9 (5 LEDs)
    // 5 bits = 0-31, perfekt für 0-29 (60 Sekunden / 2)
    // Jedes Bit repräsentiert 2 Sekunden
    int half_seconds = seconds / 2; // 0-59 wird zu 0-29

    for (int i = 0; i < 5; i++) // 5 LEDs!
    {
        if (half_seconds & (1 << i))
        {
            led_strip_set_pixel(led_strip, i + 5, 25, 0, 25);
        }
        else
        {
            led_strip_set_pixel(led_strip, i + 5, 0, 0, 0); // Off
        }
    }
    led_strip_refresh(led_strip);
}

void red_to_green_gradient(int start_time, int remaining_time)
{
    // display from led 10 to led 24 a gradient from red to green based on remaining time / start time, e.g. if remaining time is 75% of start time, show 75% green and 25% red
    float ratio = (float)remaining_time / start_time;
    for (int i = 10; i < 25; i++)
    {
        if (i < 10 + (15 * ratio))
        {
            led_strip_set_pixel(led_strip, i, 25, 0, 0); // Red
        }
        else
        {
            led_strip_set_pixel(led_strip, i, 0, 25, 0); // Green
        }
    }
    led_strip_refresh(led_strip);
}

void blink_green_led()
{
    led_set_all(0, 25, 0); // Green
    vTaskDelay(pdMS_TO_TICKS(500));
    led_clear();
    vTaskDelay(pdMS_TO_TICKS(500));
}

bool timer_started = false;
int remaining_time = 60;
int start_time = 60;

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
        if (button_increase_pressed() && button_decrease_pressed())
        {
            if (timer_started == false)
            {
                timer_started = true;
                start_time = remaining_time;
                ESP_LOGI(TAG, "Both buttons pressed! Starting timer...");
            }
            else
            {
                timer_started = false;
                remaining_time = 60;
                start_time = 60;
                ESP_LOGI(TAG, "Both buttons pressed! Stopping timer...");
            }
            led_set_all(0, 0, 50); // Blue
            vTaskDelay(pdMS_TO_TICKS(2000));
            led_clear();
        }
        else if (timer_started == false && button_increase_pressed())
        {
            ESP_LOGI(TAG, "Increasing timer!");
            led_set_all(0, 50, 0); // Green

            if (remaining_time < 60 * 15)
            {
                remaining_time += 60;
            }

            vTaskDelay(pdMS_TO_TICKS(200));
            led_clear();
        }
        else if (timer_started == false && button_decrease_pressed())
        {
            ESP_LOGI(TAG, "Decreasing timer!");
            led_set_all(50, 0, 0); // Red

            if (remaining_time > 60)
            {
                remaining_time -= 60;
            }

            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else if (timer_started)
        {
            remaining_time--;
            vTaskDelay(pdMS_TO_TICKS(1000));
            if (remaining_time == 0)
            {
                timer_started = false;
                ESP_LOGI(TAG, "Timer finished!");
            }
            else
            {
                display_time(remaining_time);
                red_to_green_gradient(start_time, remaining_time);
            }
        }
        else if (remaining_time == 0)
        {
            blink_green_led();
        }
        else
        {
            display_time(remaining_time);
            vTaskDelay(pdMS_TO_TICKS(100));
            led_clear(); // LEDs off
        }

        ESP_LOGI(TAG, "Timer started: %s, Remaining time: Minutes: %d : Seconds: %d", timer_started ? "YES" : "NO", remaining_time / 60, remaining_time % 60);
    }
}
