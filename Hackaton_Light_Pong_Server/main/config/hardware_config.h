/**
 * @file hardware_config.h
 * @brief Hardware pin and DMX configuration
 */

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* DMX Configuration for Clownfish ESP32-C3
 * IMPORTANT: Do NOT use GPIO18/19 - those are USB D-/D+ pins!
 * Using GPIO18/19 will disable USB and prevent reprogramming!
 */
#define DMX_TX_PIN GPIO_NUM_21    // 21 Wichtig!
#define DMX_RX_PIN GPIO_NUM_20    // 20 Wichtig!
#define DMX_ENABLE_PIN GPIO_NUM_9 // 9 Wichtig!

/* MH X25 DMX Configuration */
#define MH_X25_START_CHANNEL 1 // DMX start address (channels 1-12)

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_CONFIG_H
