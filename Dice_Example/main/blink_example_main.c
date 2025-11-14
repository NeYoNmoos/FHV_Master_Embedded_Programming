/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include <string.h>

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state)
    {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    }
    else
    {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    configure_led();

    /* --- I2C + ICM-42688-P minimal setup --- */
    // Assumptions: sensor is on the same I2C bus and uses pull-ups on board.
    // If your board uses different pins, change I2C_MASTER_SDA_IO / SCL below.
#define I2C_MASTER_SCL_IO 5 // SCL pin
#define I2C_MASTER_SDA_IO 4 // SDA pin
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000
#define ICM42688_ADDR 0x68 // AD0 low -> 0x68, AD0 high -> 0x69
#define ICM_WHO_AM_I_REG 0x75
#define ICM_ACCEL_XOUT_H 0x1F

    static esp_err_t i2c_master_init(void)
    {
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = I2C_MASTER_SDA_IO,
            .scl_io_num = I2C_MASTER_SCL_IO,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_MASTER_FREQ_HZ,
        };
        esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
        if (err != ESP_OK)
            return err;
        return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    }

    static esp_err_t icm_read_whoami(uint8_t *whoami)
    {
        uint8_t reg = ICM_WHO_AM_I_REG;
        return i2c_master_write_read_device(I2C_MASTER_NUM, ICM42688_ADDR, &reg, 1, whoami, 1, pdMS_TO_TICKS(1000));
    }

    static esp_err_t icm_read_accel(int16_t *ax, int16_t *ay, int16_t *az)
    {
        uint8_t reg = ICM_ACCEL_XOUT_H;
        uint8_t data[6];
        esp_err_t err = i2c_master_write_read_device(I2C_MASTER_NUM, ICM42688_ADDR, &reg, 1, data, 6, pdMS_TO_TICKS(1000));
        if (err != ESP_OK)
            return err;
        *ax = (int16_t)((data[0] << 8) | data[1]);
        *ay = (int16_t)((data[2] << 8) | data[3]);
        *az = (int16_t)((data[4] << 8) | data[5]);
        return ESP_OK;
    }

    // Simple task that initializes I2C, checks WHO_AM_I and periodically prints accel values
    static void icm_task(void *arg)
    {
        ESP_LOGI(TAG, "ICM task starting");
        if (i2c_master_init() != ESP_OK)
        {
            ESP_LOGE(TAG, "I2C init failed");
            vTaskDelete(NULL);
            return;
        }

        uint8_t who = 0;
        if (icm_read_whoami(&who) == ESP_OK)
        {
            ESP_LOGI(TAG, "WHO_AM_I = 0x%02X", who);
        }
        else
        {
            ESP_LOGW(TAG, "Could not read WHO_AM_I (device may be absent or on different pins)");
        }

        const float accel_scale = 2.0f / 32768.0f; // assuming default ±2g full scale

        while (1)
        {
            int16_t ax, ay, az;
            if (icm_read_accel(&ax, &ay, &az) == ESP_OK)
            {
                float gx = ax * accel_scale; // in g
                float gy = ay * accel_scale;
                float gz = az * accel_scale;
                ESP_LOGI(TAG, "Accel raw: %6d %6d %6d  g: %.3f %.3f %.3f", ax, ay, az, gx, gy, gz);
            }
            else
            {
                ESP_LOGW(TAG, "Failed to read accel (check wiring / address)");
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    xTaskCreate(icm_task, "icm_task", 4096, NULL, 5, NULL);

    while (1)
    {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
