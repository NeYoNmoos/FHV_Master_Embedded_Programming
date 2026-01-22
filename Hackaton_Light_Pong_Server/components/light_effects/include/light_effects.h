/**
 * @file light_effects.h
 * @author Matthias Hefel
 * @date 2026
 * @brief Light effects and animations for game events
 */

#ifndef LIGHT_EFFECTS_H
#define LIGHT_EFFECTS_H

#include <stdint.h>
#include "mh_x25_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Play winning animation for the victor
     *
     * @param winning_player Player number who won (1 or 2)
     * @param light_handle Handle to MH X25 light
     */
    void play_winning_animation(uint8_t winning_player, mh_x25_handle_t light_handle);

#ifdef __cplusplus
}
#endif

#endif // LIGHT_EFFECTS_H
