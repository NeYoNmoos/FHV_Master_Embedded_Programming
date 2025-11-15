#include "sdkconfig.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "dmx_controller.h"
#include "esp_log.h"

uint8_t dmxData[DMX_CHANNELS];

static const char *TAG = "DMX";

void dmx_init(void){
    ESP_LOGI(TAG, "Initialisiere DMX-Treiber...");

    uart_driver_delete(DMX_UART_NUM);

    uart_config_t uart_config = {
    .baud_rate = DMX_BAUDRATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_2,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(DMX_UART_NUM, 1024, 0, 0, NULL, 0));

    ESP_ERROR_CHECK(uart_param_config(DMX_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(DMX_UART_NUM, DMX_TX_PIN, DMX_RX_PIN, DE_PIN, UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(uart_set_mode(DMX_UART_NUM, UART_MODE_RS485_HALF_DUPLEX));



    //Sendemodus einschalten durch High im DE pin
    gpio_reset_pin(DE_PIN);
    gpio_set_direction(DE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DE_PIN, 1);

    dmx_clear_all();
    ESP_LOGI(TAG, "DMX-Treiber initialisiert");
}

void send_dmx_frame(const uint8_t *data, size_t length){
    //88us Break
    uart_set_line_inverse(DMX_UART_NUM, UART_SIGNAL_TXD_INV);
    esp_rom_delay_us(88);
    uart_set_line_inverse(DMX_UART_NUM, 0);

    //Mark after Break mindestens 8us
    esp_rom_delay_us(12);

    //Senden Aktivieren durch Driver enable high
    //gpio_set_level(DE_PIN,1);

    //Start code für DMX 0x00 länge 1 Byte senden
    uint8_t start_code = 0x00;
    uart_write_bytes(DMX_UART_NUM, (const char*)&start_code, 1);
    //ESP_ERROR_CHECK(uart_wait_tx_done(DMX_UART_NUM, 100));

    //Send data as a C-String with the use of a pointer, because uart doesn't expect uint8_t
    uart_write_bytes(DMX_UART_NUM, (const char*)data, length);
    //ESP_ERROR_CHECK(uart_wait_tx_done(DMX_UART_NUM, 100));

    //Senden deaktivieren
    //gpio_set_level(DE_PIN, 0);
}

void dmx_clear_all(void) {
    for (int i = 0; i < DMX_CHANNELS; i++) {
        dmxData[i] = 0;
    }
}
