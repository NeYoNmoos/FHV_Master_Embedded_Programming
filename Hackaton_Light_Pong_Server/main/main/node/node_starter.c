#include "node_starter.h"
#include "freertos/FreeRTOS.h"
#include "wifi.h"
#include "mqtt.h"
#include "sensor_node.h"
#include "types/headlight/headlight_node.h"
#include "types/led_ws2812/led_node.h"

#undef TAG
#define TAG "node_starter"

#define WIFI_SSID "labs@fhv.at"
#define WIFI_PASSWORD "vZDjRViutq9lSJ"

static const bool DISABLE_WIFI_CONNECT = false;
static const bool DISABLE_MQTT_CONNECT = false;

static void pre_configuration() 
{
    // 1. Configuration of Wi-Fi and connection build-up in parallel
    #pragma region Wi-Fi

    if (!DISABLE_WIFI_CONNECT)
    {
        ESP_ERROR_CHECK(init());

        esp_err_t ret = connect(WIFI_SSID, WIFI_PASSWORD);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to connect to Wi-Fi network with SSID '%s'", WIFI_SSID);
        }

        // FOR TESTING (show some access point info)
        wifi_ap_record_t ap_info;
        ret = esp_wifi_sta_get_ap_info(&ap_info);
        if (ret == ESP_ERR_WIFI_CONN)
        {
            ESP_LOGE(TAG, "Wi-Fi station interface not initialized");
        } else if (ret == ESP_ERR_WIFI_NOT_CONNECT) {
            ESP_LOGE(TAG, "Wi-Fi station is not connected");
        } else {
            ESP_LOGI(TAG, "--- Access Point Information ---");
            ESP_LOG_BUFFER_HEX("MAC Address", ap_info.bssid, sizeof(ap_info.bssid));
            ESP_LOG_BUFFER_CHAR("SSID", ap_info.ssid, sizeof(ap_info.ssid));
            ESP_LOGI(TAG, "Primary Channel: %d", ap_info.primary);
            ESP_LOGI(TAG, "RSSI: %d", ap_info.rssi);
        }
        // FOR TESTING (show some access point info)
    }

    #pragma endregion

    // 2. Build-up of MQTT connection
    // ToDo for 1. & 2.: Automatically restore connection if suddenly disconnected (+ again establish connection to MQTT broker)
    #pragma region MQTT

    if (!DISABLE_MQTT_CONNECT)
    {
        mqtt_connect();
    }

    #pragma endregion
}

void start_node(char* node_type)
{
    pre_configuration();

    if (strcmp(node_type, "sensor") == 0) 
    {
        ESP_LOGI(TAG, "Starting ESP32 as sensor node...");
        run_as_sensor_node();
    } 
    else if (strcmp(node_type, "headlight") == 0) 
    {
        ESP_LOGI(TAG, "Starting ESP32 as headlight node...");
        run_as_headlight_node();
    } 
    else if (strcmp(node_type, "led") == 0) 
    {
        ESP_LOGI(TAG, "Starting ESP32 as led node...");
        run_as_led_node();
    } 
    else 
    {
        ESP_LOGE(TAG, "Node type not implemented. Exiting...");
    }
}