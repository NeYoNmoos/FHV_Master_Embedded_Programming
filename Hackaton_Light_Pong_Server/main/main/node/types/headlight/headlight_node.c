
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "cJSON.h"
#include "sdkconfig.h"
#include "dmx/dmx_controller.h"
#include "dmx/headlight_controller.h"
#include "actor.h"
#include "mqtt.h"

static const char *TAG = "Headlight-Node";

static moving_head_t* moving_head = NULL;

void (*handle_actor_command)(const char* topic, const char* data, int data_len);

static void handle_headlight_command(const char* topic, const char* data, int data_len){
    // Parse the JSON payload
    cJSON *root = cJSON_ParseWithLength(data, data_len);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse actor command JSON");
        return;
    }

    // Extract common fields
    cJSON *nodeId = cJSON_GetObjectItem(root, "nodeId");
    cJSON *actorId = cJSON_GetObjectItem(root, "actorId");
    cJSON *command = cJSON_GetObjectItem(root, "command");
    cJSON *value = cJSON_GetObjectItem(root, "value");
    
    if (nodeId == NULL || actorId == NULL || command == NULL || value == NULL) {
        ESP_LOGE(TAG, "Missing required fields in actor command");
        cJSON_Delete(root);
        return;
    }

    ESP_LOGI(TAG, "Processing command: %s", command->valuestring);

    // Remove the trailing slashes from topic checks
    if(strstr(topic, "command/move") != NULL){
        ESP_LOGI(TAG, "Processing move command");
        cJSON *pan_value = cJSON_GetObjectItem(value, "pan");
        cJSON *tilt_value = cJSON_GetObjectItem(value, "tilt");
        if(pan_value != NULL && cJSON_IsNumber(pan_value)){
            uint8_t pan = (uint8_t)pan_value->valueint;
            set_position_pan(moving_head, pan);
            ESP_LOGI(TAG, "Set pan to: %d", pan);
        }
        
        if(tilt_value != NULL && cJSON_IsNumber(tilt_value)){
            uint8_t tilt = (uint8_t)tilt_value->valueint;
            set_position_tilt(moving_head, tilt);
            ESP_LOGI(TAG, "Set tilt to: %d", tilt);
        }
    }
    else if(strstr(topic, "command/rgb") != NULL){  // Removed trailing slash
        ESP_LOGI(TAG, "Processing RGB command");
        if(cJSON_IsObject(value)) {
            // Extract RGB values from object
            cJSON *r_val = cJSON_GetObjectItem(value, "r");
            cJSON *g_val = cJSON_GetObjectItem(value, "g");
            cJSON *b_val = cJSON_GetObjectItem(value, "b");
            
            if(r_val && g_val && b_val && cJSON_IsNumber(r_val) && cJSON_IsNumber(g_val) && cJSON_IsNumber(b_val)) {
                uint8_t r = (uint8_t)r_val->valueint;
                uint8_t g = (uint8_t)g_val->valueint;
                uint8_t b = (uint8_t)b_val->valueint;
                
                set_rgb_color(moving_head, r, g, b);
                ESP_LOGI(TAG, "Set RGB to: R=%d, G=%d, B=%d", r, g, b);
            } else {
                ESP_LOGE(TAG, "Invalid RGB values");
            }
        } else {
            ESP_LOGE(TAG, "RGB value is not an object");
        }
    }
    else if(strstr(topic, "command/dimmer") != NULL){
        ESP_LOGI(TAG, "Processing dimmer command");
        // The dimmer value is directly in the "value" field, not nested
        if(cJSON_IsNumber(value)) {
            uint8_t dimmer = (uint8_t)value->valueint;
            
            set_dimmer(moving_head, dimmer);
            ESP_LOGI(TAG, "Set dimmer to: %d", dimmer);
        } else {
            ESP_LOGE(TAG, "Dimmer value is not a number");
        }
    }
    else if(strstr(topic, "command/effect") != NULL){
        ESP_LOGI(TAG, "Processing Effect command");
        // The effect value is directly in the "value" field
        if(cJSON_IsNumber(value)) {
            uint8_t effect = (uint8_t)value->valueint;
            
            set_effect(moving_head, effect);
            ESP_LOGI(TAG, "Set effect to: %d", effect);
        } else {
            ESP_LOGE(TAG, "Effect value is not a number");
        }
    }
    else {
        ESP_LOGW(TAG, "Unknown command topic: %s", topic);
    }

    cJSON_Delete(root);
    generate_dmx_data(moving_head, dmxData);
    send_dmx_frame(dmxData, 11);
}

void run_as_headlight_node(void){

    handle_actor_command = handle_headlight_command;
    dmx_init();

    moving_head = get_data();
    cJSON* root = cJSON_CreateObject();

    while (1) {
        generate_dmx_data(moving_head, dmxData);
        send_dmx_frame(dmxData, 11);
        mqtt_async_publish_to("actors/headlight/1", cJSON_PrintUnformatted(root));
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/*
set_effect(moving_head, EFFECT_NEUTRAL);
    set_dimmer(moving_head, 255);
    set_rgb_color(moving_head, 255,0,0);

    while (1) {
        set_position(moving_head,0x40, 0x40);
        generate_dmx_data(moving_head, dmxData);
        send_dmx_frame(dmxData, 11);
        vTaskDelay(pdMS_TO_TICKS(5000));

        set_position(moving_head,0x80, 0x80);
        generate_dmx_data(moving_head, dmxData);
        send_dmx_frame(dmxData, 11);
        vTaskDelay(pdMS_TO_TICKS(5000));

        set_position(moving_head,0xC0, 0xC0);
        generate_dmx_data(moving_head, dmxData);
        send_dmx_frame(dmxData, 11);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
*/