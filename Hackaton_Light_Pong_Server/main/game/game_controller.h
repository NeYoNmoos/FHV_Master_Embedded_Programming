/**
 * @file game_controller.h
 * @author Matthias Hefel
 * @date 2026
 * @brief Main game controller task for Light Pong
 */

#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include <stdint.h>
#include "dmx_driver.h"
#include "mh_x25_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Main game controller task
     *
     * @param pvParameters Task parameters (unused)
     */
    void dmx_controller_task(void *pvParameters);

    /**
     * @brief Set game controller context
     *
     * @param light MH X25 light handle
     * @param events Event group for paddle hits
     * @param side Pointer to current side state
     * @param btn_left Pointer to left button state
     * @param btn_right Pointer to right button state
     * @param score Pointer to game score
     */
    void game_controller_set_context(mh_x25_handle_t light,
                                     EventGroupHandle_t events,
                                     volatile int *side,
                                     volatile uint8_t *btn_left,
                                     volatile uint8_t *btn_right,
                                     void *score);

#ifdef __cplusplus
}
#endif

#endif // GAME_CONTROLLER_H
