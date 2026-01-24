/**
 * @file game_config.h
 * @author Matthias Hefel
 * @date 2026
 * @brief Game configuration constants for Light Pong
 */

#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include "freertos/FreeRTOS.h"
#include "espnow_handler.h" // For PADDLE_TOP_HIT and PADDLE_BOTTOM_HIT

#ifdef __cplusplus
extern "C"
{
#endif

/* Game State */
#define SIDE_TOP 0
#define SIDE_BOTTOM 1

/* Button States */
#define BUTTON_FIREBALL 0
#define BUTTON_NORMAL 1

// Win condition
#define WIN_SCORE 9

// Timeout configuration
#define HIT_TIMEOUT_MS 2000
#define CELEBRATION_BLINKS 10
#define CELEBRATION_BLINK_ON_MS 250
#define CELEBRATION_BLINK_OFF_MS 250

// Playing field boundaries
#define PAN_MIN (128 - 20)     // Left corner
#define PAN_MAX (128 + 20)     // Right corner
#define TILT_TOP (128 + 60)    // Top border
#define TILT_BOTTOM (128 - 60) // Bottom border

#ifdef __cplusplus
}
#endif

#endif // GAME_CONFIG_H
