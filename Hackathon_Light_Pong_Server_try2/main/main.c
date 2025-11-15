#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dmx/dmx_controller.h"
#include "dmx/mh_x25_controller.h"
#include "esp_log.h"

static const char *TAG = "MH_X25_TILT_TEST";

void app_main(void)
{
    dmx_init();
    mh_x25_t *head = mh_x25_get_data();

    ESP_LOGI(TAG, "Starting simple tilt test.");

    // Set some defaults: Pan to center, shutter open, color white
    mh_x25_set_position(head, 128, 0); // Pan center, tilt start
    mh_x25_set_shutter(head, SHUTTER_OPEN);
    mh_x25_set_color(head, COLOR_WHITE);
    mh_x25_set_gobo(head, GOBO_OPEN);

    while (1)
    {
        // Tilt up
        ESP_LOGI(TAG, "Tilting up...");
        for (int i = 0; i <= 255; i++)
        {
            head->tilt = i;
            mh_x25_generate_dmx_data(head, dmxData);
            send_dmx_frame(dmxData, 7);
            vTaskDelay(pdMS_TO_TICKS(20)); // 20ms delay for smooth movement
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait a second at the top

        // Tilt down
        ESP_LOGI(TAG, "Tilting down...");
        for (int i = 255; i >= 0; i--)
        {
            head->tilt = i;
            mh_x25_generate_dmx_data(head, dmxData);
            send_dmx_frame(dmxData, 7);
            vTaskDelay(pdMS_TO_TICKS(20)); // 20ms delay for smooth movement
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait a second at the bottom
    }
}
