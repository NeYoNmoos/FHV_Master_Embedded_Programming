#include "mh_x25_controller.h"

static mh_x25_t mh_x25_head;

void mh_x25_generate_dmx_data(const mh_x25_t *config, uint8_t *dmx_data)
{
    // DMX channels are 1-based, arrays are 0-based.
    // dmx_data[0] is for channel 1, dmx_data[1] for channel 2, etc.
    dmx_data[0] = config->pan;
    dmx_data[1] = config->tilt;
    dmx_data[2] = config->color;
    dmx_data[3] = config->shutter;
    dmx_data[4] = config->gobo;
    dmx_data[5] = config->gobo_rotation;
}

void mh_x25_set_position(mh_x25_t *head, uint8_t pan, uint8_t tilt)
{
    head->pan = pan;
    head->tilt = tilt;
}

void mh_x25_set_color(mh_x25_t *head, mh_x25_color_t color)
{
    head->color = color;
}

void mh_x25_set_shutter(mh_x25_t *head, mh_x25_shutter_t shutter)
{
    head->shutter = shutter;
}

void mh_x25_set_gobo(mh_x25_t *head, mh_x25_gobo_t gobo)
{
    head->gobo = gobo;
}

void mh_x25_set_gobo_rotation(mh_x25_t *head, mh_x25_gobo_rot_t rotation, uint8_t value)
{
    if (rotation == GOBO_ROT_FIXED)
    {
        head->gobo_rotation = value > 63 ? 63 : value;
    }
    else
    {
        head->gobo_rotation = rotation + (value > 84 ? 84 : value);
    }
}

mh_x25_t *mh_x25_get_data(void)
{
    return &mh_x25_head;
}
