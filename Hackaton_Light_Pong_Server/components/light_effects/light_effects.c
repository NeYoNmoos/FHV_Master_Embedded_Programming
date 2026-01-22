/**
 * @file light_effects.c
 * @brief Light effects and animations implementation
 */

#include "light_effects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "light_effects";

// Winning animation function
void play_winning_animation(uint8_t winning_player, mh_x25_handle_t light_handle)
{
    ESP_LOGI(TAG, "🏆 PLAYER %d WINS! Playing victory animation...", winning_player);

    // Set winning color
    uint8_t win_color = (winning_player == 1) ? MH_X25_COLOR_GREEN : MH_X25_COLOR_DARK_BLUE;

    // Victory animation: Fast color cycling with rotation
    for (int cycle = 0; cycle < 3; cycle++)
    {
        // Cycle through colors rapidly
        uint8_t colors[] = {MH_X25_COLOR_RED, MH_X25_COLOR_GREEN, MH_X25_COLOR_DARK_BLUE,
                            MH_X25_COLOR_YELLOW, MH_X25_COLOR_PINK, MH_X25_COLOR_LIGHT_BLUE};

        for (int i = 0; i < 6; i++)
        {
            mh_x25_set_color(light_handle, colors[i]);
            mh_x25_set_gobo_rotation(light_handle, 200); // Fast rotation
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    // Flash winning color with gobo effects
    mh_x25_set_color(light_handle, win_color);
    mh_x25_set_gobo_rotation(light_handle, 0);

    for (int i = 0; i < 8; i++)
    {
        // Alternate between different gobos
        mh_x25_set_gobo(light_handle, (i % 4) + 1); // Cycle through gobos 1-4
        mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
        vTaskDelay(pdMS_TO_TICKS(150));
        mh_x25_set_dimmer(light_handle, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    // Final celebration: Spin and flash
    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
    mh_x25_set_gobo_rotation(light_handle, 200);
    for (int i = 0; i < 5; i++)
    {
        mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
        vTaskDelay(pdMS_TO_TICKS(300));
        mh_x25_set_dimmer(light_handle, 0);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    // Reset light to default state
    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
    mh_x25_set_gobo_rotation(light_handle, 0);

    ESP_LOGI(TAG, "🎊 Victory animation complete! Resetting game...");
}
