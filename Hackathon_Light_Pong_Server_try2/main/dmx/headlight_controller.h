#ifndef HEADLIGHT_CONTROLLER_H
#define HEADLIGHT_CONTROLLER_H

#include "sdkconfig.h"
#include <stdint.h>
#include <stdbool.h>
#include "dmx_controller.h"

// #define DMX_CHANNELS CONFIG_DMX_CHANNELS

typedef struct {
    // unsinged interger size 8 typed (normed that the size is correct over different systems) -> 0-255 
    uint8_t pan_low;
    uint8_t tilt_low;

    //Turnspeed
    uint8_t pan_tilt_speed;

    //Colors 0-255
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    //Farbmakros?
    uint8_t color_makro;

    //LED-Schaltung, Strobe-Effect, Neutral, Reset
    uint8_t effects;

    //Dimmerintensität in Prozent
    uint8_t dimmer;

    uint8_t pan_high;
    uint8_t tilt_high;
} moving_head_t;

// Farbmakro-Definitionen
typedef enum {
    COLOR_NEUTRAL = 0,
    COLOR_MACRO_1 = 41,
    COLOR_MACRO_2 = 81,
    COLOR_MACRO_3 = 121,
    COLOR_MACRO_4 = 141,
    COLOR_MACRO_5 = 161,
    COLOR_MACRO_6 = 201,
    COLOR_NEUTRAL_END = 241
} color_macro_t;

// Effekt-Definitionen
typedef enum {
    EFFECT_LED_OFF = 0,
    EFFECT_NEUTRAL = 11,
    EFFECT_RESET = 21,
    EFFECT_STROBE = 31,
    EFFECT_RANDOM_STROBE = 201,
    EFFECT_NEUTRAL_END = 251
} effect_t;

// Funktionsprototypen
void generate_dmx_data(const moving_head_t* config, uint8_t* dmx_data);
void set_position(moving_head_t* head, uint8_t pan, uint8_t tilt);
void set_position_tilt(moving_head_t* head, uint8_t pan);
void set_position_pan(moving_head_t* head, uint8_t tilt);
void set_rgb_color(moving_head_t* head, uint8_t red, uint8_t green, uint8_t blue);
void set_color_macro(moving_head_t* head, color_macro_t macro);
void set_effect(moving_head_t* head, effect_t effect);
void set_dimmer(moving_head_t* head, uint8_t intensity);
void set_speed(moving_head_t* head, uint8_t speed);

moving_head_t* get_data(void);

#endif // MOVING_HEAD_H