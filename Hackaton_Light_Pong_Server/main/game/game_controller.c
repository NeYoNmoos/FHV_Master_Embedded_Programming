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
#include "espnow_handler.h"
#include "esp_log.h"
#include "esp_random.h"
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

// Configuration for each player/side
typedef struct
{
    int side_id;
    int opposite_side;
    uint8_t opposite_tilt;
    EventBits_t event_bit;
    volatile uint8_t *button_state;
    uint8_t player_number;
    uint8_t celebration_color;
} side_config_t;

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

static uint8_t get_random_pan(uint8_t pan_min, uint8_t pan_max)
{
    return pan_min + (esp_random() % (pan_max - pan_min + 1));
}

static void apply_ball_effect(uint8_t button_pressed)
{
    if (button_pressed == BUTTON_FIREBALL)
    {
        ESP_LOGI(TAG, "Fireball activated");
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
}

static void celebration_blink(uint8_t color)
{
    mh_x25_set_color(light_handle, color);
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
}

static bool handle_paddle_hit(const side_config_t *cfg, uint8_t pan_min, uint8_t pan_max, TickType_t timeout)
{
    if (cfg == NULL)
    {
        ESP_LOGE(TAG, "Invalid config pointer");
        return false;
    }

    ESP_LOGI(TAG, "Waiting for Player %d paddle hit", cfg->player_number);

    xEventGroupClearBits(paddle_events, cfg->event_bit);
    EventBits_t bits = xEventGroupWaitBits(
        paddle_events,
        cfg->event_bit,
        pdTRUE,
        pdFALSE,
        timeout);

    if (bits & cfg->event_bit)
    {
        ESP_LOGI(TAG, "Player %d hit detected", cfg->player_number);
        apply_ball_effect(*cfg->button_state);

        uint8_t pan_position = get_random_pan(pan_min, pan_max);
        mh_x25_set_position_16bit(light_handle, pan_position << 8, cfg->opposite_tilt << 8);
        *current_side = cfg->opposite_side;

        vTaskDelay(pdMS_TO_TICKS(1000));
        return true;
    }

    return false;
}

static bool handle_timeout(const side_config_t *cfg, uint8_t pan_min, uint8_t pan_max)
{
    if (cfg == NULL)
    {
        ESP_LOGE(TAG, "Invalid config pointer");
        return false;
    }

    if (cfg->player_number == 1)
        game_score->score_2++;
    else
        game_score->score_1++;

    ESP_LOGI(TAG, "Timeout: Player %d missed - Score P1=%d P2=%d",
             cfg->player_number, game_score->score_1, game_score->score_2);

    esp_err_t ret = espnow_broadcast_score(game_score, sizeof(game_score_t));
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to send score update: %s", esp_err_to_name(ret));
    }

    uint8_t winner = (game_score->score_1 >= WIN_SCORE) ? 1 : (game_score->score_2 >= WIN_SCORE) ? 2
                                                                                                 : 0;

    if (winner > 0)
    {
        play_winning_animation(winner, light_handle);
        game_score->score_1 = 0;
        game_score->score_2 = 0;
        ret = espnow_broadcast_score(game_score, sizeof(game_score_t));
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "Failed to send reset score: %s", esp_err_to_name(ret));
        }

        uint8_t pan_position = get_random_pan(pan_min, pan_max);
        mh_x25_set_position_16bit(light_handle, pan_position << 8, TILT_TOP << 8);
        *current_side = SIDE_TOP;
        vTaskDelay(pdMS_TO_TICKS(2000));
        return true;
    }

    celebration_blink(cfg->celebration_color);
    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
    vTaskDelay(pdMS_TO_TICKS(500));

    handle_paddle_hit(cfg, pan_min, pan_max, portMAX_DELAY);
    return false;
}

void dmx_controller_task(void *pvParameters)
{
    const uint8_t pan_min = PAN_MIN;
    const uint8_t pan_max = PAN_MAX;
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
    uint8_t pan_position = get_random_pan(pan_min, pan_max);
    ESP_LOGI(TAG, "Game started: ball at TOP (pan=%d, tilt=%d)", pan_position, TILT_TOP);
    mh_x25_set_position_16bit(light_handle, pan_position << 8, TILT_TOP << 8);
    vTaskDelay(pdMS_TO_TICKS(1000));

    side_config_t player1_config = {
        .side_id = SIDE_TOP,
        .opposite_side = SIDE_BOTTOM,
        .opposite_tilt = TILT_BOTTOM,
        .event_bit = PADDLE_TOP_HIT,
        .button_state = last_btn_left_pressed,
        .player_number = 1,
        .celebration_color = MH_X25_COLOR_DARK_BLUE};

    side_config_t player2_config = {
        .side_id = SIDE_BOTTOM,
        .opposite_side = SIDE_TOP,
        .opposite_tilt = TILT_TOP,
        .event_bit = PADDLE_BOTTOM_HIT,
        .button_state = last_btn_right_pressed,
        .player_number = 2,
        .celebration_color = MH_X25_COLOR_GREEN};

    while (1)
    {
        side_config_t *current_player = (*current_side == SIDE_TOP) ? &player1_config : &player2_config;

        if (!handle_paddle_hit(current_player, pan_min, pan_max, timeout_ticks))
        {
            if (handle_timeout(current_player, pan_min, pan_max))
            {
                continue;
            }
        }
    }
}
