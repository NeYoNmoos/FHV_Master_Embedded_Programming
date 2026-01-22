/**
 * @file dmx_driver.c
 * @author Matthias Hefel
 * @date 2026
 * @brief DMX512 driver implementation for RS-485 communication
 */

#include "dmx_driver.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_gpio.h"

static const char *TAG = "DMX";

#define DMX_TX_BUFFER_SIZE 1024
#define DMX_RX_BUFFER_SIZE 256 // Minimum required by UART driver (even in TX-only mode)
#define DMX_TASK_STACK_SIZE 4096
#define DMX_TASK_PRIORITY 5
#define DMX_UPDATE_RATE_HZ 44 // Standard DMX refresh rate

/**
 * @brief DMX driver context structure
 */
typedef struct
{
    uart_port_t uart_num;
    gpio_num_t tx_pin;
    gpio_num_t rx_pin;
    gpio_num_t enable_pin;
    uint16_t universe_size;
    uint8_t *dmx_data;
    SemaphoreHandle_t mutex;
    TaskHandle_t tx_task_handle;
    bool is_running;
} dmx_context_t;

/**
 * @brief Send DMX break signal
 */
static esp_err_t dmx_send_break(dmx_context_t *ctx)
{
    uart_wait_tx_done(ctx->uart_num, portMAX_DELAY);
    uart_set_line_inverse(ctx->uart_num, UART_SIGNAL_TXD_INV);
    esp_rom_delay_us(DMX_BREAK_US);

    uart_set_line_inverse(ctx->uart_num, UART_SIGNAL_INV_DISABLE);
    esp_rom_delay_us(DMX_MAB_US);

    return ESP_OK;
}

/**
 * @brief Continuous transmission task
 */
static void dmx_tx_task(void *arg)
{
    dmx_context_t *ctx = (dmx_context_t *)arg;
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1000 / DMX_UPDATE_RATE_HZ);

    ESP_LOGI(TAG, "DMX transmission task started");

    while (ctx->is_running)
    {
        if (dmx_transmit(ctx) != ESP_OK)
        {
            ESP_LOGW(TAG, "DMX transmission failed");
        }

        vTaskDelayUntil(&last_wake_time, period);
    }

    ESP_LOGI(TAG, "DMX transmission task stopped");
    vTaskDelete(NULL);
}

esp_err_t dmx_init(const dmx_config_t *config, dmx_handle_t *out_handle)
{
    if (config == NULL || out_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->universe_size == 0 || config->universe_size > DMX_UNIVERSE_SIZE)
    {
        ESP_LOGE(TAG, "Invalid universe size: %d", config->universe_size);
        return ESP_ERR_INVALID_ARG;
    }

    dmx_context_t *ctx = (dmx_context_t *)calloc(1, sizeof(dmx_context_t));
    if (ctx == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate DMX context");
        return ESP_ERR_NO_MEM;
    }

    ctx->dmx_data = (uint8_t *)calloc(config->universe_size + 1, sizeof(uint8_t));
    if (ctx->dmx_data == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate DMX data buffer");
        free(ctx);
        return ESP_ERR_NO_MEM;
    }

    ctx->mutex = xSemaphoreCreateMutex();
    if (ctx->mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        free(ctx->dmx_data);
        free(ctx);
        return ESP_ERR_NO_MEM;
    }

    ctx->uart_num = config->uart_num;
    ctx->tx_pin = config->tx_pin;
    ctx->rx_pin = config->rx_pin;
    ctx->enable_pin = config->enable_pin;
    ctx->universe_size = config->universe_size;
    ctx->is_running = false;
    ctx->tx_task_handle = NULL;

    ctx->dmx_data[0] = 0x00;

    // Configure RS-485 enable pin
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << ctx->enable_pin),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE};
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure enable pin");
        vSemaphoreDelete(ctx->mutex);
        free(ctx->dmx_data);
        free(ctx);
        return ret;
    }

    // Set RS-485 to transmit mode
    gpio_set_level(ctx->enable_pin, 1);

    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = DMX_BAUD_RATE,
        .data_bits = DMX_DATA_BITS,
        .parity = DMX_PARITY,
        .stop_bits = DMX_STOP_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ret = uart_param_config(ctx->uart_num, &uart_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "UART param config failed");
        gpio_reset_pin(ctx->enable_pin);
        vSemaphoreDelete(ctx->mutex);
        free(ctx->dmx_data);
        free(ctx);
        return ret;
    }

    ret = uart_set_pin(ctx->uart_num, ctx->tx_pin, ctx->rx_pin,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "UART set pin failed");
        gpio_reset_pin(ctx->enable_pin);
        vSemaphoreDelete(ctx->mutex);
        free(ctx->dmx_data);
        free(ctx);
        return ret;
    }

    ret = uart_driver_install(ctx->uart_num, DMX_RX_BUFFER_SIZE,
                              DMX_TX_BUFFER_SIZE, 0, NULL, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "UART driver install failed");
        gpio_reset_pin(ctx->enable_pin);
        vSemaphoreDelete(ctx->mutex);
        free(ctx->dmx_data);
        free(ctx);
        return ret;
    }

    *out_handle = (dmx_handle_t)ctx;
    ESP_LOGI(TAG, "DMX initialized: UART%d, TX:%d, EN:%d, Channels:%d",
             ctx->uart_num, ctx->tx_pin, ctx->enable_pin, ctx->universe_size);

    return ESP_OK;
}

esp_err_t dmx_deinit(dmx_handle_t handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    dmx_context_t *ctx = (dmx_context_t *)handle;

    if (ctx->is_running)
    {
        dmx_stop_transmission(handle);
    }

    uart_driver_delete(ctx->uart_num);
    gpio_reset_pin(ctx->enable_pin);

    vSemaphoreDelete(ctx->mutex);
    free(ctx->dmx_data);
    free(ctx);

    ESP_LOGI(TAG, "DMX deinitialized");
    return ESP_OK;
}

esp_err_t dmx_set_channel(dmx_handle_t handle, uint16_t channel, uint8_t value)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    dmx_context_t *ctx = (dmx_context_t *)handle;

    if (channel == 0 || channel > ctx->universe_size)
    {
        ESP_LOGE(TAG, "Invalid channel: %d (valid: 1-%d)", channel, ctx->universe_size);
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE)
    {
        ctx->dmx_data[channel] = value;
        xSemaphoreGive(ctx->mutex);
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t dmx_set_channels(dmx_handle_t handle, uint16_t start_channel,
                           const uint8_t *data, uint16_t length)
{
    if (handle == NULL || data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    dmx_context_t *ctx = (dmx_context_t *)handle;

    if (start_channel == 0 || start_channel > ctx->universe_size ||
        (start_channel + length - 1) > ctx->universe_size)
    {
        ESP_LOGE(TAG, "Invalid channel range: %d-%d", start_channel, start_channel + length - 1);
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE)
    {
        memcpy(&ctx->dmx_data[start_channel], data, length);
        xSemaphoreGive(ctx->mutex);
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t dmx_get_channel(dmx_handle_t handle, uint16_t channel, uint8_t *value)
{
    if (handle == NULL || value == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    dmx_context_t *ctx = (dmx_context_t *)handle;

    if (channel == 0 || channel > ctx->universe_size)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE)
    {
        *value = ctx->dmx_data[channel];
        xSemaphoreGive(ctx->mutex);
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t dmx_transmit(dmx_handle_t handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    dmx_context_t *ctx = (dmx_context_t *)handle;
    esp_err_t ret;

    ret = dmx_send_break(ctx);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE)
    {
        int bytes_written = uart_write_bytes(ctx->uart_num, ctx->dmx_data,
                                             ctx->universe_size + 1);
        xSemaphoreGive(ctx->mutex);

        if (bytes_written != (ctx->universe_size + 1))
        {
            ESP_LOGW(TAG, "DMX write incomplete: %d/%d bytes",
                     bytes_written, ctx->universe_size + 1);
            return ESP_FAIL;
        }

        uart_wait_tx_done(ctx->uart_num, pdMS_TO_TICKS(DMX_PACKET_TIMEOUT_MS));
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t dmx_start_transmission(dmx_handle_t handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    dmx_context_t *ctx = (dmx_context_t *)handle;

    if (ctx->is_running)
    {
        ESP_LOGW(TAG, "DMX transmission already running");
        return ESP_ERR_INVALID_STATE;
    }

    ctx->is_running = true;

    BaseType_t ret = xTaskCreate(dmx_tx_task, "dmx_tx", DMX_TASK_STACK_SIZE,
                                 ctx, DMX_TASK_PRIORITY, &ctx->tx_task_handle);

    if (ret != pdPASS)
    {
        ctx->is_running = false;
        ESP_LOGE(TAG, "Failed to create DMX transmission task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "DMX continuous transmission started");
    return ESP_OK;
}

esp_err_t dmx_stop_transmission(dmx_handle_t handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    dmx_context_t *ctx = (dmx_context_t *)handle;

    if (!ctx->is_running)
    {
        return ESP_OK;
    }

    ctx->is_running = false;

    if (ctx->tx_task_handle != NULL)
    {
        vTaskDelay(pdMS_TO_TICKS(50));
        ctx->tx_task_handle = NULL;
    }

    ESP_LOGI(TAG, "DMX continuous transmission stopped");
    return ESP_OK;
}

esp_err_t dmx_clear_all(dmx_handle_t handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    dmx_context_t *ctx = (dmx_context_t *)handle;

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE)
    {
        memset(&ctx->dmx_data[1], 0, ctx->universe_size);
        xSemaphoreGive(ctx->mutex);
        ESP_LOGI(TAG, "All DMX channels cleared");
        return ESP_OK;
    }

    return ESP_FAIL;
}
