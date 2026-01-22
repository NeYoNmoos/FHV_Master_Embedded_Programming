/**
 * @file light_effects.c
 * @author Matthias Hefel
 * @date 2026
 * @brief Light effects and animations implementation for MH-X25
 */

#include "light_effects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "light_effects";

// Winning animation function
void play_winning_animation(uint8_t winning_player, mh_x25_handle_t light_handle)
{
    ESP_LOGI(TAG, "Player %d wins - starting victory animation", winning_player);

    uint8_t win_color = (winning_player == 1) ? MH_X25_COLOR_GREEN : MH_X25_COLOR_DARK_BLUE;

    for (int cycle = 0; cycle < 3; cycle++)
    {
        uint8_t colors[] = {MH_X25_COLOR_RED, MH_X25_COLOR_GREEN, MH_X25_COLOR_DARK_BLUE,
                            MH_X25_COLOR_YELLOW, MH_X25_COLOR_PINK, MH_X25_COLOR_LIGHT_BLUE};

        for (int i = 0; i < 6; i++)
        {
            mh_x25_set_color(light_handle, colors[i]);
            mh_x25_set_gobo_rotation(light_handle, 200);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    mh_x25_set_color(light_handle, win_color);
    mh_x25_set_gobo_rotation(light_handle, 0);

    for (int i = 0; i < 8; i++)
    {
        mh_x25_set_gobo(light_handle, (i % 4) + 1);
        mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
        vTaskDelay(pdMS_TO_TICKS(150));
        mh_x25_set_dimmer(light_handle, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
    mh_x25_set_gobo_rotation(light_handle, 200);
    for (int i = 0; i < 5; i++)
    {
        mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
        vTaskDelay(pdMS_TO_TICKS(300));
        mh_x25_set_dimmer(light_handle, 0);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
    mh_x25_set_gobo_rotation(light_handle, 0);

    ESP_LOGI(TAG, "Victory animation complete, resetting game");
}
