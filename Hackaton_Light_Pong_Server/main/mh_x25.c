/**
 * @file mh_x25.c
 * @brief MH X25 LED Moving Head Light Control Implementation
 */

#include "mh_x25.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "MH_X25";

/**
 * @brief MH X25 device context
 */
typedef struct
{
    dmx_handle_t dmx_handle;               // DMX driver handle
    uint16_t start_channel;                // DMX start channel
    uint8_t channels[MH_X25_NUM_CHANNELS]; // Current channel values
} mh_x25_context_t;

esp_err_t mh_x25_init(const mh_x25_config_t *config, mh_x25_handle_t *out_handle)
{
    if (config == NULL || out_handle == NULL)
    {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    if (config->dmx_handle == NULL)
    {
        ESP_LOGE(TAG, "Invalid DMX handle");
        return ESP_ERR_INVALID_ARG;
    }

    if (config->start_channel == 0 || config->start_channel > (512 - MH_X25_NUM_CHANNELS + 1))
    {
        ESP_LOGE(TAG, "Invalid start channel: %d", config->start_channel);
        return ESP_ERR_INVALID_ARG;
    }

    // Allocate context
    mh_x25_context_t *ctx = (mh_x25_context_t *)calloc(1, sizeof(mh_x25_context_t));
    if (ctx == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate context");
        return ESP_ERR_NO_MEM;
    }

    ctx->dmx_handle = config->dmx_handle;
    ctx->start_channel = config->start_channel;

    // Initialize all channels to 0
    memset(ctx->channels, 0, MH_X25_NUM_CHANNELS);

    // Write initial values to DMX
    dmx_set_channels(ctx->dmx_handle, ctx->start_channel, ctx->channels, MH_X25_NUM_CHANNELS);

    *out_handle = (mh_x25_handle_t)ctx;

    ESP_LOGI(TAG, "MH X25 initialized: DMX channels %d-%d",
             ctx->start_channel, ctx->start_channel + MH_X25_NUM_CHANNELS - 1);

    return ESP_OK;
}

esp_err_t mh_x25_deinit(mh_x25_handle_t handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    mh_x25_context_t *ctx = (mh_x25_context_t *)handle;

    // Turn off the light before cleanup
    mh_x25_off(handle);

    free(ctx);
    ESP_LOGI(TAG, "MH X25 deinitialized");

    return ESP_OK;
}

esp_err_t mh_x25_set_pan(mh_x25_handle_t handle, uint8_t pan)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    mh_x25_context_t *ctx = (mh_x25_context_t *)handle;
    ctx->channels[MH_X25_CHANNEL_PAN] = pan;

    return dmx_set_channel(ctx->dmx_handle,
                           ctx->start_channel + MH_X25_CHANNEL_PAN,
                           pan);
}

esp_err_t mh_x25_set_tilt(mh_x25_handle_t handle, uint8_t tilt)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    mh_x25_context_t *ctx = (mh_x25_context_t *)handle;
    ctx->channels[MH_X25_CHANNEL_TILT] = tilt;

    return dmx_set_channel(ctx->dmx_handle,
                           ctx->start_channel + MH_X25_CHANNEL_TILT,
                           tilt);
}

esp_err_t mh_x25_set_position(mh_x25_handle_t handle, uint8_t pan, uint8_t tilt)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    mh_x25_context_t *ctx = (mh_x25_context_t *)handle;

    ctx->channels[MH_X25_CHANNEL_PAN] = pan;
    ctx->channels[MH_X25_CHANNEL_TILT] = tilt;

    uint8_t pos_data[2] = {pan, tilt};

    return dmx_set_channels(ctx->dmx_handle,
                            ctx->start_channel + MH_X25_CHANNEL_PAN,
                            pos_data, 2);
}

esp_err_t mh_x25_set_color(mh_x25_handle_t handle, uint8_t color)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    mh_x25_context_t *ctx = (mh_x25_context_t *)handle;
    ctx->channels[MH_X25_CHANNEL_COLOR] = color;

    return dmx_set_channel(ctx->dmx_handle,
                           ctx->start_channel + MH_X25_CHANNEL_COLOR,
                           color);
}

esp_err_t mh_x25_set_shutter(mh_x25_handle_t handle, uint8_t shutter)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    mh_x25_context_t *ctx = (mh_x25_context_t *)handle;
    ctx->channels[MH_X25_CHANNEL_SHUTTER] = shutter;

    return dmx_set_channel(ctx->dmx_handle,
                           ctx->start_channel + MH_X25_CHANNEL_SHUTTER,
                           shutter);
}

esp_err_t mh_x25_set_gobo(mh_x25_handle_t handle, uint8_t gobo)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    mh_x25_context_t *ctx = (mh_x25_context_t *)handle;
    ctx->channels[MH_X25_CHANNEL_GOBO] = gobo;

    return dmx_set_channel(ctx->dmx_handle,
                           ctx->start_channel + MH_X25_CHANNEL_GOBO,
                           gobo);
}

esp_err_t mh_x25_set_gobo_rotation(mh_x25_handle_t handle, uint8_t rotation)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    mh_x25_context_t *ctx = (mh_x25_context_t *)handle;
    ctx->channels[MH_X25_CHANNEL_GOBO_ROT] = rotation;

    return dmx_set_channel(ctx->dmx_handle,
                           ctx->start_channel + MH_X25_CHANNEL_GOBO_ROT,
                           rotation);
}

esp_err_t mh_x25_set_all(mh_x25_handle_t handle, uint8_t pan, uint8_t tilt,
                         uint8_t color, uint8_t shutter, uint8_t gobo, uint8_t gobo_rot)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    mh_x25_context_t *ctx = (mh_x25_context_t *)handle;

    ctx->channels[MH_X25_CHANNEL_PAN] = pan;
    ctx->channels[MH_X25_CHANNEL_TILT] = tilt;
    ctx->channels[MH_X25_CHANNEL_COLOR] = color;
    ctx->channels[MH_X25_CHANNEL_SHUTTER] = shutter;
    ctx->channels[MH_X25_CHANNEL_GOBO] = gobo;
    ctx->channels[MH_X25_CHANNEL_GOBO_ROT] = gobo_rot;

    return dmx_set_channels(ctx->dmx_handle, ctx->start_channel,
                            ctx->channels, MH_X25_NUM_CHANNELS);
}

esp_err_t mh_x25_off(mh_x25_handle_t handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Blackout: center position, shutter closed, white color
    ESP_LOGI("mh_x25", "Turning off light - setting shutter to blackout");
    return mh_x25_set_all(handle, 128, 128, MH_X25_COLOR_WHITE,
                          MH_X25_SHUTTER_BLACKOUT, MH_X25_GOBO_OPEN, MH_X25_GOBO_ROT_STOP);
}
