/**
 * @file game_config.h
 * @brief Game configuration constants
 */

#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Game State */
#define SIDE_TOP 0
#define SIDE_BOTTOM 1

// Event bits for paddle hits
#define PADDLE_TOP_HIT BIT0
#define PADDLE_BOTTOM_HIT BIT1

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

// Player MAC addresses
#define PLAYER1_MAC \
    {               \
        0xCC, 0xBA, 0x97, 0x95, 0xD5, 0xC0}
#define PLAYER2_MAC \
    {               \
        0xF0, 0xF5, 0xBD, 0xB3, 0xB8, 0x44}

#ifdef __cplusplus
}
#endif

#endif // GAME_CONFIG_H
