/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "esp_log.h"

// SHTC3 Sensor Configuration
#define SHTC3_I2C_ADDR 0x70       // SHTC3 I2C address
#define I2C_MASTER_SCL_IO 6       // GPIO for I2C SCL (try GPIO6 - common on ESP32-C3)
#define I2C_MASTER_SDA_IO 5       // GPIO for I2C SDA (try GPIO5 - common on ESP32-C3)
#define I2C_MASTER_NUM I2C_NUM_0  // I2C port number
#define I2C_MASTER_FREQ_HZ 100000 // I2C master clock frequency (100kHz)

// SHTC3 Commands
#define SHTC3_CMD_WAKEUP 0x3517  // Wake up command
#define SHTC3_CMD_SLEEP 0xB098   // Sleep command
#define SHTC3_CMD_READ_ID 0xEFC8 // Read ID command
#define SHTC3_CMD_MEASURE 0x7CA2 // Normal mode measurement (T first, clock stretching disabled)

static const char *TAG = "SHTC3";

/**
 * @brief Initialize I2C master
 */
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
    {
        return err;
    }

    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

/**
 * @brief Send a command to SHTC3 sensor
 */
static esp_err_t shtc3_send_command(uint16_t command)
{
    uint8_t cmd_buffer[2];
    cmd_buffer[0] = (command >> 8) & 0xFF; // MSB
    cmd_buffer[1] = command & 0xFF;        // LSB

    return i2c_master_write_to_device(I2C_MASTER_NUM, SHTC3_I2C_ADDR,
                                      cmd_buffer, 2, pdMS_TO_TICKS(1000));
}

/**
 * @brief Soft reset the SHTC3 sensor
 */
static esp_err_t shtc3_soft_reset(void)
{
    uint8_t cmd_buffer[2] = {0x80, 0x5D}; // Soft reset command
    esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, SHTC3_I2C_ADDR,
                                               cmd_buffer, 2, pdMS_TO_TICKS(1000));
    if (err == ESP_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(1)); // Wait 1ms for reset to complete
    }
    return err;
}

/**
 * @brief Read temperature and humidity from SHTC3
 */
static esp_err_t shtc3_read_temp_humidity(float *temperature, float *humidity)
{
    uint8_t data[6]; // 2 bytes temp + 1 byte CRC + 2 bytes humidity + 1 byte CRC
    esp_err_t err;

    // Wake up the sensor - retry once if it fails
    err = shtc3_send_command(SHTC3_CMD_WAKEUP);
    if (err != ESP_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        err = shtc3_send_command(SHTC3_CMD_WAKEUP);
        if (err != ESP_OK)
        {
            return err;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(1)); // Wait 1ms after wakeup

    // Send measurement command
    err = shtc3_send_command(SHTC3_CMD_MEASURE);
    if (err != ESP_OK)
    {
        shtc3_send_command(SHTC3_CMD_SLEEP);
        return err;
    }

    // Wait for measurement to complete (max 12.6ms according to datasheet)
    vTaskDelay(pdMS_TO_TICKS(15));

    // Read the measurement data
    err = i2c_master_read_from_device(I2C_MASTER_NUM, SHTC3_I2C_ADDR,
                                      data, 6, pdMS_TO_TICKS(1000));
    if (err != ESP_OK)
    {
        shtc3_send_command(SHTC3_CMD_SLEEP);
        return err;
    }

    // Put sensor back to sleep to save power
    shtc3_send_command(SHTC3_CMD_SLEEP);

    // Convert temperature (first 2 bytes)
    uint16_t temp_raw = (data[0] << 8) | data[1];
    *temperature = -45.0f + 175.0f * ((float)temp_raw / 65535.0f);

    // Convert humidity (bytes 3 and 4)
    uint16_t hum_raw = (data[3] << 8) | data[4];
    *humidity = 100.0f * ((float)hum_raw / 65535.0f);

    return ESP_OK;
}

/**
 * @brief Scan I2C bus for devices (wake SHTC3 first)
 */
static void i2c_scanner(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");

    // Wake up SHTC3 before scanning (it may be asleep)
    uint8_t wakeup_cmd[2] = {0x35, 0x17};
    i2c_master_write_to_device(I2C_MASTER_NUM, SHTC3_I2C_ADDR, wakeup_cmd, 2, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(1));

    uint8_t devices_found = 0;

    // Only check SHTC3 address to avoid bus errors
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, SHTC3_I2C_ADDR,
                                               wakeup_cmd, 2, pdMS_TO_TICKS(100));

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Found SHTC3 sensor at address: 0x%02X", SHTC3_I2C_ADDR);
        devices_found++;
    }

    if (devices_found == 0)
    {
        ESP_LOGW(TAG, "SHTC3 sensor not responding! Check your wiring:");
        ESP_LOGW(TAG, "  - SDA should be connected to GPIO%d", I2C_MASTER_SDA_IO);
        ESP_LOGW(TAG, "  - SCL should be connected to GPIO%d", I2C_MASTER_SCL_IO);
        ESP_LOGW(TAG, "  - Check power (3.3V) and ground connections");
    }
}

void app_main(void)
{
    // Initialize I2C
    ESP_LOGI(TAG, "Initializing I2C master...");
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C initialization failed!");
        return;
    }
    ESP_LOGI(TAG, "I2C initialized successfully");
    ESP_LOGI(TAG, "Using SDA: GPIO%d, SCL: GPIO%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    // Perform a soft reset of the sensor
    ESP_LOGI(TAG, "Resetting SHTC3 sensor...");
    ret = shtc3_soft_reset();
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "SHTC3 reset successful");
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // Scan for I2C devices
    i2c_scanner();

    ESP_LOGI(TAG, "Starting sensor readings...");
    vTaskDelay(pdMS_TO_TICKS(500));

    // Main loop - read sensor every 2 seconds
    while (1)
    {
        float temperature = 0.0;
        float humidity = 0.0;

        // Read temperature and humidity
        ret = shtc3_read_temp_humidity(&temperature, &humidity);

        if (ret == ESP_OK)
        {
            // Print the results to console
            printf("\n");
            printf("=========================================\n");
            printf("  Temperature: %.2f °C\n", temperature);
            printf("  Humidity:    %.2f %%\n", humidity);
            printf("=========================================\n");

            ESP_LOGI(TAG, "Temperature: %.2f °C, Humidity: %.2f %%",
                     temperature, humidity);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
