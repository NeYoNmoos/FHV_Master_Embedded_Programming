#include <string.h>
#include "node_starter.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#undef TAG
#define TAG "main"

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("device", NVS_READONLY, &handle));

    char key[] = "node_type";
    char value[32];
    size_t size = sizeof(value);
    esp_err_t err = nvs_get_str(handle, key, value, &size);

    switch (err) 
    {
        case ESP_OK:
            ESP_LOGI(TAG, "Read %s = %s", key, value);
            start_node(value);
            break;

        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "The value is not initialized yet!");
            break;

        default:
            ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
    }
}