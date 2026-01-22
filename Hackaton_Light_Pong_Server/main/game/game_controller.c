/**
 * @file game_controller.c
 * @author Matthias Hefel
 * @date 2026
 * @brief Main game controller implementation for Light Pong
 */

#include "game_controller.h"
#include "game_types.h"
#include "../config/game_config.h"
#include "light_effects.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "game_controller";

// Context variables
static mh_x25_handle_t light_handle = NULL;
static EventGroupHandle_t paddle_events = NULL;
static volatile uint8_t *last_btn_left_pressed = NULL;
static volatile uint8_t *last_btn_right_pressed = NULL;
static volatile int *current_side = NULL;
static game_score_t *game_score = NULL;

void game_controller_set_context(mh_x25_handle_t light,
                                 EventGroupHandle_t events,
                                 volatile int *side,
                                 volatile uint8_t *btn_left,
                                 volatile uint8_t *btn_right,
                                 void *score)
{
    light_handle = light;
    paddle_events = events;
    current_side = side;
    last_btn_left_pressed = btn_left;
    last_btn_right_pressed = btn_right;
    game_score = (game_score_t *)score;
}

void dmx_controller_task(void *pvParameters)
{

    const uint8_t pan_min = PAN_MIN;
    const uint8_t pan_max = PAN_MAX;
    const uint8_t tilt_top = TILT_TOP;
    const uint8_t tilt_bottom = TILT_BOTTOM;
    const TickType_t timeout_ticks = pdMS_TO_TICKS(HIT_TIMEOUT_MS);

    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
    mh_x25_set_shutter(light_handle, MH_X25_SHUTTER_OPEN);
    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
    mh_x25_set_gobo_rotation(light_handle, 0);
    mh_x25_set_speed(light_handle, MH_X25_SPEED_FAST);
    mh_x25_set_special(light_handle, MH_X25_SPECIAL_NO_BLACKOUT_PAN_TILT);

    vTaskDelay(pdMS_TO_TICKS(500));

    *current_side = SIDE_TOP;
    uint8_t pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
    ESP_LOGI(TAG, "Game started: ball at TOP (pan=%d, tilt=%d)", pan_position, tilt_top);
    mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1)
    {
        EventBits_t bits;

        if (*current_side == SIDE_TOP)
        {
            ESP_LOGI(TAG, "Waiting for Player 1 paddle hit");

            xEventGroupClearBits(paddle_events, PADDLE_TOP_HIT);

            bits = xEventGroupWaitBits(
                paddle_events,
                PADDLE_TOP_HIT,
                pdTRUE,
                pdFALSE,
                timeout_ticks);

            if (bits & PADDLE_TOP_HIT)
            {
                ESP_LOGI(TAG, "Player 1 hit detected, moving ball to BOTTOM");

                if (*last_btn_left_pressed == 0)
                {
                    ESP_LOGI(TAG, "Fireball activated by Player 1");
                    mh_x25_set_color(light_handle, MH_X25_COLOR_RED);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4);
                    mh_x25_set_gobo_rotation(light_handle, 200);
                }
                else
                {
                    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                    mh_x25_set_gobo_rotation(light_handle, 0);
                }

                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                ESP_LOGI(TAG, "Moving to BOTTOM (pan=%d, tilt=%d)", pan_position, tilt_bottom);

                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_bottom << 8);
                *current_side = SIDE_BOTTOM;

                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            else
            {
                game_score->score_2++;
                ESP_LOGI(TAG, "Timeout: Player 1 missed - Score P1=%d P2=%d",
                         game_score->score_1, game_score->score_2);

                esp_now_send(NULL, (uint8_t *)game_score, sizeof(game_score_t));

                if (game_score->score_2 >= WIN_SCORE)
                {
                    play_winning_animation(2, light_handle);

                    game_score->score_1 = 0;
                    game_score->score_2 = 0;
                    esp_now_send(NULL, (uint8_t *)game_score, sizeof(game_score_t));

                    pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                    mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                    *current_side = SIDE_TOP;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    continue;
                }

                mh_x25_set_color(light_handle, MH_X25_COLOR_DARK_BLUE);
                mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                mh_x25_set_gobo_rotation(light_handle, 0);

                for (int i = 0; i < CELEBRATION_BLINKS; i++)
                {
                    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
                    vTaskDelay(pdMS_TO_TICKS(CELEBRATION_BLINK_ON_MS));
                    mh_x25_set_dimmer(light_handle, 0);
                    vTaskDelay(pdMS_TO_TICKS(CELEBRATION_BLINK_OFF_MS));
                }
                mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);

                mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                vTaskDelay(pdMS_TO_TICKS(500));

                ESP_LOGI(TAG, "Waiting for Player 1 hit (no timeout)");
                xEventGroupClearBits(paddle_events, PADDLE_TOP_HIT);
                bits = xEventGroupWaitBits(
                    paddle_events,
                    PADDLE_TOP_HIT,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY);

                ESP_LOGI(TAG, "Player 1 hit detected");

                if (*last_btn_left_pressed == 0)
                {
                    mh_x25_set_color(light_handle, MH_X25_COLOR_RED);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4);
                    mh_x25_set_gobo_rotation(light_handle, 200);
                }
                else
                {
                    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                    mh_x25_set_gobo_rotation(light_handle, 0);
                }

                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_bottom << 8);
                *current_side = SIDE_BOTTOM;

                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
        else
        {
            ESP_LOGI(TAG, "Waiting for Player 2 paddle hit");

            xEventGroupClearBits(paddle_events, PADDLE_BOTTOM_HIT);

            bits = xEventGroupWaitBits(
                paddle_events,
                PADDLE_BOTTOM_HIT,
                pdTRUE,
                pdFALSE,
                timeout_ticks);

            if (bits & PADDLE_BOTTOM_HIT)
            {
                ESP_LOGI(TAG, "Player 2 hit detected, moving ball to TOP");

                if (*last_btn_right_pressed == 0)
                {
                    ESP_LOGI(TAG, "Fireball activated by Player 2");
                    mh_x25_set_color(light_handle, MH_X25_COLOR_RED);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4);
                    mh_x25_set_gobo_rotation(light_handle, 200);
                }
                else
                {
                    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                    mh_x25_set_gobo_rotation(light_handle, 0);
                }

                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                *current_side = SIDE_TOP;

                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            else
            {
                game_score->score_1++;
                ESP_LOGI(TAG, "Timeout: Player 2 missed - Score P1=%d P2=%d",
                         game_score->score_1, game_score->score_2);

                esp_now_send(NULL, (uint8_t *)game_score, sizeof(game_score_t));

                if (game_score->score_1 >= WIN_SCORE)
                {
                    play_winning_animation(1, light_handle);

                    game_score->score_1 = 0;
                    game_score->score_2 = 0;
                    esp_now_send(NULL, (uint8_t *)game_score, sizeof(game_score_t));

                    pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                    mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                    *current_side = SIDE_TOP;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    continue;
                }

                // Victory celebration - Blink GREEN for Player 1 (5 seconds)
                mh_x25_set_color(light_handle, MH_X25_COLOR_GREEN);
                mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                mh_x25_set_gobo_rotation(light_handle, 0);

                for (int i = 0; i < CELEBRATION_BLINKS; i++)
                {
                    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
                    vTaskDelay(pdMS_TO_TICKS(CELEBRATION_BLINK_ON_MS));
                    mh_x25_set_dimmer(light_handle, 0);
                    vTaskDelay(pdMS_TO_TICKS(CELEBRATION_BLINK_OFF_MS));
                }
                mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);

                mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                vTaskDelay(pdMS_TO_TICKS(500));

                ESP_LOGI(TAG, "Waiting for Player 2 hit (no timeout)");
                xEventGroupClearBits(paddle_events, PADDLE_BOTTOM_HIT);
                bits = xEventGroupWaitBits(
                    paddle_events,
                    PADDLE_BOTTOM_HIT,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY);

                ESP_LOGI(TAG, "Player 2 hit detected");

                if (*last_btn_right_pressed == 0)
                {
                    ESP_LOGI(TAG, "Fireball activated by Player 2");
                    mh_x25_set_color(light_handle, MH_X25_COLOR_RED);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4);
                    mh_x25_set_gobo_rotation(light_handle, 200);
                }
                else
                {
                    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                    mh_x25_set_gobo_rotation(light_handle, 0);
                }

                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                *current_side = SIDE_TOP;

                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}
