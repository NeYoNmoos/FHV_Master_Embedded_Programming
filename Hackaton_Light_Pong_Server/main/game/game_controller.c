/**
 * @file game_controller.c
 * @brief Main game controller implementation
 */

#include "game_controller.h"
#include "game_types.h"
#include "../config/game_config.h"
#include "../effects/light_effects.h"
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

// Task 2: Light Pong Game Controller
void dmx_controller_task(void *pvParameters)
{
    ESP_LOGI(TAG, "🎮 Light Pong Game Controller started");
    ESP_LOGI(TAG, "==============================");

    // Playing field boundaries
    const uint8_t pan_min = PAN_MIN;
    const uint8_t pan_max = PAN_MAX;
    const uint8_t tilt_top = TILT_TOP;
    const uint8_t tilt_bottom = TILT_BOTTOM;

    // Timeout for scoring (2 seconds)
    const TickType_t timeout_ticks = pdMS_TO_TICKS(HIT_TIMEOUT_MS);

    // Set up light - white color, open shutter, and full dimmer
    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
    mh_x25_set_shutter(light_handle, MH_X25_SHUTTER_OPEN);
    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL); // Make sure light is on
    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
    mh_x25_set_gobo_rotation(light_handle, 0);
    mh_x25_set_speed(light_handle, MH_X25_SPEED_FAST);
    mh_x25_set_special(light_handle, MH_X25_SPECIAL_NO_BLACKOUT_PAN_TILT);

    vTaskDelay(pdMS_TO_TICKS(500));

    // Start ball at TOP with random pan position
    *current_side = SIDE_TOP;
    uint8_t pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
    ESP_LOGI(TAG, "⬆️  Ball starting at TOP position (pan=%d, tilt=%d)...", pan_position, tilt_top);
    mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1)
    {
        EventBits_t bits;

        if (*current_side == SIDE_TOP)
        {
            // Ball is at TOP, wait for LEFT paddle (ID=1) to hit
            ESP_LOGI(TAG, "⬆️  Ball at TOP - waiting for LEFT paddle (ID=1)...");

            // Clear any pre-registered hits from when it wasn't this player's turn
            xEventGroupClearBits(paddle_events, PADDLE_TOP_HIT);

            bits = xEventGroupWaitBits(
                paddle_events,
                PADDLE_TOP_HIT,
                pdTRUE,       // Clear bit after reading
                pdFALSE,      // Wait for any bit (only one in this case)
                timeout_ticks // Wait with timeout
            );

            if (bits & PADDLE_TOP_HIT)
            {
                ESP_LOGI(TAG, "LEFT PADDLE HIT! Ball moving to BOTTOM...");

                // Check if button was pressed for fireball effect
                if (*last_btn_left_pressed == 0)
                {
                    ESP_LOGI(TAG, "🔥 FIREBALL ACTIVATED by Player 1!");
                    mh_x25_set_color(light_handle, MH_X25_COLOR_RED);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4); // Fireball gobo
                    mh_x25_set_gobo_rotation(light_handle, 200);  // Fast rotation
                }
                else
                {
                    // Normal ball appearance
                    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                    mh_x25_set_gobo_rotation(light_handle, 0);
                }

                // Generate random pan position for BOTTOM
                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                ESP_LOGI(TAG, "Moving to BOTTOM (pan=%d, tilt=%d)", pan_position, tilt_bottom);

                // Move ball to BOTTOM with random pan
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_bottom << 8);
                *current_side = SIDE_BOTTOM;

                vTaskDelay(pdMS_TO_TICKS(1000)); // Movement delay
            }
            else
            {
                // Timeout! Player 1 (LEFT/TOP) missed - Player 2 scores
                game_score->score_2++;
                ESP_LOGI(TAG, "⏱TIMEOUT! Player 1 missed. Score: Player 1 = %d, Player 2 = %d",
                         game_score->score_1, game_score->score_2);

                // Send score to clients
                esp_now_send(NULL, (uint8_t *)game_score, sizeof(game_score_t));

                // Check for win condition
                if (game_score->score_2 >= WIN_SCORE)
                {
                    play_winning_animation(2, light_handle);

                    // Reset game
                    game_score->score_1 = 0;
                    game_score->score_2 = 0;
                    esp_now_send(NULL, (uint8_t *)game_score, sizeof(game_score_t));

                    // Reset ball to starting position (TOP)
                    pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                    mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                    *current_side = SIDE_TOP;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    continue; // Skip to next iteration
                }

                // Victory celebration - Blink BLUE for Player 2 (5 seconds)
                mh_x25_set_color(light_handle, MH_X25_COLOR_DARK_BLUE);
                mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                mh_x25_set_gobo_rotation(light_handle, 0);

                for (int i = 0; i < CELEBRATION_BLINKS; i++)
                {
                    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL); // ON
                    vTaskDelay(pdMS_TO_TICKS(CELEBRATION_BLINK_ON_MS));
                    mh_x25_set_dimmer(light_handle, 0); // OFF
                    vTaskDelay(pdMS_TO_TICKS(CELEBRATION_BLINK_OFF_MS));
                }
                mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL); // End with light ON

                // Ball stays at TOP - reset to white, same player tries again
                mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                // current_side remains SIDE_TOP - no position change needed
                vTaskDelay(pdMS_TO_TICKS(500));

                // Now wait INDEFINITELY for player to hit (no more timeouts)
                ESP_LOGI(TAG, "⬆Waiting for Player 1 to hit (no timeout)...");
                xEventGroupClearBits(paddle_events, PADDLE_TOP_HIT);
                bits = xEventGroupWaitBits(
                    paddle_events,
                    PADDLE_TOP_HIT,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY // Wait forever - no more point loss
                );

                ESP_LOGI(TAG, "Player 1 HIT! Ball moving to BOTTOM...");

                // Check if button was pressed for fireball effect
                if (*last_btn_left_pressed == 0)
                {
                    ESP_LOGI(TAG, "FIREBALL ACTIVATED by Player 1!");
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

                // Generate random pan position for BOTTOM
                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                ESP_LOGI(TAG, "Moving to BOTTOM (pan=%d, tilt=%d)", pan_position, tilt_bottom);

                // Move ball to BOTTOM with random pan
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_bottom << 8);
                *current_side = SIDE_BOTTOM;

                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
        else
        { // current_side == SIDE_BOTTOM
            // Ball is at BOTTOM, wait for RIGHT paddle (ID=2) to hit
            ESP_LOGI(TAG, "⬇Ball at BOTTOM - waiting for RIGHT paddle (ID=2)...");

            // Clear any pre-registered hits from when it wasn't this player's turn
            xEventGroupClearBits(paddle_events, PADDLE_BOTTOM_HIT);

            bits = xEventGroupWaitBits(
                paddle_events,
                PADDLE_BOTTOM_HIT,
                pdTRUE, // Clear bit after reading
                pdFALSE,
                timeout_ticks // Wait with timeout
            );

            if (bits & PADDLE_BOTTOM_HIT)
            {
                ESP_LOGI(TAG, "RIGHT PADDLE HIT! Ball moving to TOP...");

                // Check if button was pressed for fireball effect
                if (*last_btn_right_pressed == 0)
                {
                    ESP_LOGI(TAG, "🔥 FIREBALL ACTIVATED by Player 2!");
                    mh_x25_set_color(light_handle, MH_X25_COLOR_RED);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4); // Fireball gobo
                    mh_x25_set_gobo_rotation(light_handle, 200);  // Fast rotation
                }
                else
                {
                    // Normal ball appearance
                    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                    mh_x25_set_gobo_rotation(light_handle, 0);
                }

                // Generate random pan position for TOP
                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                ESP_LOGI(TAG, "Moving to TOP (pan=%d, tilt=%d)", pan_position, tilt_top);

                // Move ball to TOP with random pan
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                *current_side = SIDE_TOP;

                vTaskDelay(pdMS_TO_TICKS(1000)); // Movement delay
            }
            else
            {
                // Timeout! Player 2 (RIGHT/BOTTOM) missed - Player 1 scores
                game_score->score_1++;
                ESP_LOGI(TAG, "TIMEOUT! Player 2 missed. Score: Player 1 = %d, Player 2 = %d",
                         game_score->score_1, game_score->score_2);

                // Send score to clients
                esp_now_send(NULL, (uint8_t *)game_score, sizeof(game_score_t));

                // Check for win condition
                if (game_score->score_1 >= WIN_SCORE)
                {
                    play_winning_animation(1, light_handle);

                    // Reset game
                    game_score->score_1 = 0;
                    game_score->score_2 = 0;
                    esp_now_send(NULL, (uint8_t *)game_score, sizeof(game_score_t));

                    // Reset ball to starting position (TOP)
                    pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                    mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                    *current_side = SIDE_TOP;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    continue; // Skip to next iteration
                }

                // Victory celebration - Blink GREEN for Player 1 (5 seconds)
                mh_x25_set_color(light_handle, MH_X25_COLOR_GREEN);
                mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                mh_x25_set_gobo_rotation(light_handle, 0);

                for (int i = 0; i < CELEBRATION_BLINKS; i++)
                {
                    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL); // ON
                    vTaskDelay(pdMS_TO_TICKS(CELEBRATION_BLINK_ON_MS));
                    mh_x25_set_dimmer(light_handle, 0); // OFF
                    vTaskDelay(pdMS_TO_TICKS(CELEBRATION_BLINK_OFF_MS));
                }
                mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL); // End with light ON

                // Ball stays at BOTTOM - reset to white, same player tries again
                mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                // current_side remains SIDE_BOTTOM - no position change needed
                vTaskDelay(pdMS_TO_TICKS(500));

                // Now wait INDEFINITELY for player to hit (no more timeouts)
                ESP_LOGI(TAG, "⬇Waiting for Player 2 to hit (no timeout)...");
                xEventGroupClearBits(paddle_events, PADDLE_BOTTOM_HIT);
                bits = xEventGroupWaitBits(
                    paddle_events,
                    PADDLE_BOTTOM_HIT,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY // Wait forever - no more point loss
                );

                ESP_LOGI(TAG, "Player 2 HIT! Ball moving to TOP...");

                // Check if button was pressed for fireball effect
                if (*last_btn_right_pressed == 0)
                {
                    ESP_LOGI(TAG, "🔥 FIREBALL ACTIVATED by Player 2!");
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

                // Generate random pan position for TOP
                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                ESP_LOGI(TAG, "Moving to TOP (pan=%d, tilt=%d)", pan_position, tilt_top);

                // Move ball to TOP with random pan
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                *current_side = SIDE_TOP;

                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}
