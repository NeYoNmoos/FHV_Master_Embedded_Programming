#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dmx/dmx_controller.h"
#include "dmx/mh_x25_controller.h"

void app_main(void)
{
    dmx_init();
    mh_x25_t *head = mh_x25_get_data();

    while (1)
    {
        // Move to center
        mh_x25_set_position(head, 128, 128);
        mh_x25_generate_dmx_data(head, dmxData);
        send_dmx_frame(dmxData, 7);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // Change color to red and open shutter
        mh_x25_set_color(head, COLOR_RED);
        mh_x25_set_shutter(head, SHUTTER_OPEN);
        mh_x25_generate_dmx_data(head, dmxData);
        send_dmx_frame(dmxData, 7);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // Strobe effect
        mh_x25_set_shutter(head, SHUTTER_STROBE);
        mh_x25_generate_dmx_data(head, dmxData);
        send_dmx_frame(dmxData, 7);
        vTaskDelay(pdMS_TO_TICKS(5000));

        // Change color to blue
        mh_x25_set_color(head, COLOR_DARK_BLUE);
        mh_x25_set_shutter(head, SHUTTER_OPEN);
        mh_x25_generate_dmx_data(head, dmxData);
        send_dmx_frame(dmxData, 7);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // Move to a different position
        mh_x25_set_position(head, 50, 200);
        mh_x25_generate_dmx_data(head, dmxData);
        send_dmx_frame(dmxData, 7);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
