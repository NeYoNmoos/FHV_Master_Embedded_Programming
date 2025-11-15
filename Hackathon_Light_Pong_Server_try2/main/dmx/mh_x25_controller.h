#ifndef MH_X25_CONTROLLER_H
#define MH_X25_CONTROLLER_H

#include "sdkconfig.h"
#include <stdint.h>
#include <stdbool.h>
#include "dmx_controller.h"

typedef struct
{
    uint8_t pan;
    uint8_t tilt;
    uint8_t color;
    uint8_t shutter;
    uint8_t gobo;
    uint8_t gobo_rotation;
} mh_x25_t;

// Color Wheel
typedef enum
{
    COLOR_WHITE = 0,
    COLOR_YELLOW = 8,
    COLOR_PINK = 12,
    COLOR_GREEN = 17,
    COLOR_PEACHBLOW = 22,
    COLOR_LIGHT_BLUE = 27,
    COLOR_YELLOW_GREEN = 32,
    COLOR_RED = 37,
    COLOR_DARK_BLUE = 42,
    COLOR_RAINBOW_CW = 128,
    COLOR_RAINBOW_CCW = 192
} mh_x25_color_t;

// Shutter
typedef enum
{
    SHUTTER_BLACKOUT = 0,
    SHUTTER_OPEN = 4,
    SHUTTER_STROBE = 8,
    SHUTTER_OPEN_2 = 216
} mh_x25_shutter_t;

// Gobo Wheel
typedef enum
{
    GOBO_OPEN = 0,
    GOBO_2 = 8,
    GOBO_3 = 16,
    GOBO_4 = 24,
    GOBO_5 = 32,
    GOBO_6 = 40,
    GOBO_7 = 48,
    GOBO_8 = 56,
    GOBO_8_SHAKE = 64,
    GOBO_7_SHAKE = 72,
    GOBO_6_SHAKE = 80,
    GOBO_5_SHAKE = 88,
    GOBO_4_SHAKE = 96,
    GOBO_3_SHAKE = 104,
    GOBO_2_SHAKE = 112,
    GOBO_OPEN_2 = 120,
    GOBO_RAINBOW_CW = 128,
    GOBO_RAINBOW_CCW = 192
} mh_x25_gobo_t;

// Gobo Rotation
typedef enum
{
    GOBO_ROT_FIXED = 0,
    GOBO_ROT_CW = 64,
    GOBO_ROT_CCW = 148,
    GOBO_ROT_YOYO = 232
} mh_x25_gobo_rot_t;

void mh_x25_generate_dmx_data(const mh_x25_t *config, uint8_t *dmx_data);
void mh_x25_set_position(mh_x25_t *head, uint8_t pan, uint8_t tilt);
void mh_x25_set_color(mh_x25_t *head, mh_x25_color_t color);
void mh_x25_set_shutter(mh_x25_t *head, mh_x25_shutter_t shutter);
void mh_x25_set_gobo(mh_x25_t *head, mh_x25_gobo_t gobo);
void mh_x25_set_gobo_rotation(mh_x25_t *head, mh_x25_gobo_rot_t rotation, uint8_t value);

mh_x25_t *mh_x25_get_data(void);

#endif // MH_X25_CONTROLLER_H
