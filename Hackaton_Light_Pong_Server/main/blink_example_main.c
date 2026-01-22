/* DMX512 MH X25 Control Example

   This example demonstrates how to control the MH X25 LED moving head light
   via DMX512 protocol over RS-485.
*/
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "drivers/dmx/dmx.h"
#include "drivers/mh_x25/mh_x25.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "esp_system.h"

static const char *TAG = "dmx_example";

/* Game State */
#define SIDE_TOP 0
#define SIDE_BOTTOM 1

// Event bits for paddle hits
#define PADDLE_TOP_HIT BIT0
#define PADDLE_BOTTOM_HIT BIT1

static EventGroupHandle_t paddle_events;
static volatile int current_side = SIDE_TOP; // Ball starts at top

// Store button press states for fireball feature
static volatile uint8_t last_btn_left_pressed = 0;
static volatile uint8_t last_btn_right_pressed = 0;

/* DMX Configuration for Clownfish ESP32-C3
 * IMPORTANT: Do NOT use GPIO18/19 - those are USB D-/D+ pins!
 * Using GPIO18/19 will disable USB and prevent reprogramming!
 */
#define DMX_TX_PIN GPIO_NUM_21    // 21 Wichtig!
#define DMX_RX_PIN GPIO_NUM_20    // 20 Wichtig!
#define DMX_ENABLE_PIN GPIO_NUM_9 // 9 Wichtig!

/* MH X25 DMX Configuration */
#define MH_X25_START_CHANNEL 1 // DMX start address (channels 1-6)

static dmx_handle_t dmx_handle = NULL;
static mh_x25_handle_t light_handle = NULL;

typedef struct
{
    uint8_t id;
    uint8_t btn_right_pressed;
    uint8_t btn_left_pressed;
    float ax, ay, az;
    float gx, gy, gz;
} input_event_t;

typedef struct
{
    uint8_t score_1;
    uint8_t score_2;
} game_score_t;

game_score_t game_score = {0, 0}; // Initialize game score

void on_receive(const uint8_t *mac_addr, const uint8_t *data, int len)
{

    ESP_LOGI(TAG, "ESP-NOW data received, length: %d", len);
    if (len < 1)
        return; // safety check

    // Cast data to input_event_t
    input_event_t *m = (input_event_t *)data;

    // Check which paddle sent the signal based on ID
    if (m->id == 1)
    {
        last_btn_left_pressed = m->btn_right_pressed; // switch on npurpouse
        ESP_LOGI(TAG, "🎯 LEFT PADDLE (ID=1) HIT detected! Button pressed: %d", last_btn_left_pressed);

        xEventGroupSetBits(paddle_events, PADDLE_TOP_HIT);
    }
    else if (m->id == 2)
    {
        last_btn_right_pressed = m->btn_left_pressed; // switch on npurpouse
        ESP_LOGI(TAG, "🎯 RIGHT PADDLE (ID=2) HIT detected! Button pressed: %d", last_btn_right_pressed);

        xEventGroupSetBits(paddle_events, PADDLE_BOTTOM_HIT);
    }
    else
    {
        ESP_LOGW(TAG, "⚠️  Unknown paddle ID: %d", m->id);
    }
}

void add_peer(const uint8_t mac[6])
{
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, mac, 6);
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
}

// Task 1: ESP-NOW Receiver
void espnow_receiver_task(void *pvParameters)
{
    ESP_LOGI(TAG, "ESP-NOW receiver task started");

    // esp now init
    // NVS init
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    // WiFi Station Mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    // ESP-NOW init
    esp_now_init();
    esp_now_register_recv_cb((esp_now_recv_cb_t)on_receive);

    ESP_LOGI(TAG, "Master bereit...");

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    printf("ESP32-C3 WIFI MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3],
           mac[4], mac[5]);

    uint8_t mac2[6];
    esp_read_mac(mac2, ESP_MAC_WIFI_STA);
    printf("ESP32-C3 MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac2[0], mac2[1], mac2[2], mac2[3],
           mac2[4], mac2[5]);

    // Add peers AFTER ESP-NOW is initialized
    uint8_t player1_esp[] = {0xCC, 0xBA, 0x97, 0x95, 0xD5, 0xC0}; // ESP 1
    uint8_t player2_esp[] = {0xF0, 0xF5, 0xBD, 0xB3, 0xB8, 0x44}; // ESP 2

    add_peer(player1_esp);
    add_peer(player2_esp);

    ESP_LOGI(TAG, "Added ESP-NOW peers");

    while (1)
    {
        // Your ESP-NOW receiving code here
        // This runs independently

        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay
    }
}

// Winning animation function
void play_winning_animation(uint8_t winning_player)
{
    ESP_LOGI(TAG, "🏆 PLAYER %d WINS! Playing victory animation...", winning_player);

    // Set winning color
    uint8_t win_color = (winning_player == 1) ? MH_X25_COLOR_GREEN : MH_X25_COLOR_DARK_BLUE;

    // Victory animation: Fast color cycling with rotation
    for (int cycle = 0; cycle < 3; cycle++)
    {
        // Cycle through colors rapidly
        uint8_t colors[] = {MH_X25_COLOR_RED, MH_X25_COLOR_GREEN, MH_X25_COLOR_DARK_BLUE,
                            MH_X25_COLOR_YELLOW, MH_X25_COLOR_PINK, MH_X25_COLOR_LIGHT_BLUE};

        for (int i = 0; i < 6; i++)
        {
            mh_x25_set_color(light_handle, colors[i]);
            mh_x25_set_gobo_rotation(light_handle, 200); // Fast rotation
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    // Flash winning color with gobo effects
    mh_x25_set_color(light_handle, win_color);
    mh_x25_set_gobo_rotation(light_handle, 0);

    for (int i = 0; i < 8; i++)
    {
        // Alternate between different gobos
        mh_x25_set_gobo(light_handle, (i % 4) + 1); // Cycle through gobos 1-4
        mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
        vTaskDelay(pdMS_TO_TICKS(150));
        mh_x25_set_dimmer(light_handle, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    // Final celebration: Spin and flash
    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
    mh_x25_set_gobo_rotation(light_handle, 200);
    for (int i = 0; i < 5; i++)
    {
        mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
        vTaskDelay(pdMS_TO_TICKS(300));
        mh_x25_set_dimmer(light_handle, 0);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    // Reset light to default state
    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL);
    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
    mh_x25_set_gobo_rotation(light_handle, 0);

    ESP_LOGI(TAG, "🎊 Victory animation complete! Resetting game...");
}

// Task 2: Light Pong Game Controller
void dmx_controller_task(void *pvParameters)
{
    ESP_LOGI(TAG, "🎮 Light Pong Game Controller started");
    ESP_LOGI(TAG, "==============================");

    // Playing field boundaries
    const uint8_t pan_min = 128 - 20;     // Left corner
    const uint8_t pan_max = 128 + 20;     // Right corner
    const uint8_t tilt_top = 128 + 60;    // Top border
    const uint8_t tilt_bottom = 128 - 60; // Bottom border

    // Timeout for scoring (2 seconds)
    const TickType_t timeout_ticks = pdMS_TO_TICKS(2000);

    // Set up light - white color, open shutter, and full dimmer
    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
    mh_x25_set_shutter(light_handle, MH_X25_SHUTTER_OPEN);
    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL); // Make sure light is on
    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
    mh_x25_set_gobo_rotation(light_handle, 0);
    mh_x25_set_speed(light_handle, MH_X25_SPEED_FAST);
    mh_x25_set_special(light_handle, MH_X25_SPECIAL_NO_BLACKOUT_PAN_TILT);

    vTaskDelay(pdMS_TO_TICKS(500));

    // Start ball at TOP with random pan position
    current_side = SIDE_TOP;
    uint8_t pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
    ESP_LOGI(TAG, "⬆️  Ball starting at TOP position (pan=%d, tilt=%d)...", pan_position, tilt_top);
    mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1)
    {
        EventBits_t bits;

        if (current_side == SIDE_TOP)
        {
            // Ball is at TOP, wait for LEFT paddle (ID=1) to hit
            ESP_LOGI(TAG, "⬆️  Ball at TOP - waiting for LEFT paddle (ID=1)...");

            // Clear any pre-registered hits from when it wasn't this player's turn
            xEventGroupClearBits(paddle_events, PADDLE_TOP_HIT);

            bits = xEventGroupWaitBits(
                paddle_events,
                PADDLE_TOP_HIT,
                pdTRUE,       // Clear bit after reading
                pdFALSE,      // Wait for any bit (only one in this case)
                timeout_ticks // Wait with timeout
            );

            if (bits & PADDLE_TOP_HIT)
            {
                ESP_LOGI(TAG, "LEFT PADDLE HIT! Ball moving to BOTTOM...");

                // Check if button was pressed for fireball effect
                if (last_btn_left_pressed == 0)
                {
                    ESP_LOGI(TAG, "🔥 FIREBALL ACTIVATED by Player 1!");
                    mh_x25_set_color(light_handle, MH_X25_COLOR_RED);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4); // Fireball gobo
                    mh_x25_set_gobo_rotation(light_handle, 200);  // Fast rotation
                }
                else
                {
                    // Normal ball appearance
                    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                    mh_x25_set_gobo_rotation(light_handle, 0);
                }

                // Generate random pan position for BOTTOM
                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                ESP_LOGI(TAG, "Moving to BOTTOM (pan=%d, tilt=%d)", pan_position, tilt_bottom);

                // Move ball to BOTTOM with random pan
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_bottom << 8);
                current_side = SIDE_BOTTOM;

                vTaskDelay(pdMS_TO_TICKS(1000)); // Movement delay
            }
            else
            {
                // Timeout! Player 1 (LEFT/TOP) missed - Player 2 scores
                game_score.score_2++;
                ESP_LOGI(TAG, "⏱TIMEOUT! Player 1 missed. Score: Player 1 = %d, Player 2 = %d",
                         game_score.score_1, game_score.score_2);

                // Send score to clients
                esp_now_send(NULL, (uint8_t *)&game_score, sizeof(game_score_t));

                // Check for win condition
                if (game_score.score_2 >= 9)
                {
                    play_winning_animation(2);

                    // Reset game
                    game_score.score_1 = 0;
                    game_score.score_2 = 0;
                    esp_now_send(NULL, (uint8_t *)&game_score, sizeof(game_score_t));

                    // Reset ball to starting position (TOP)
                    pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                    mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                    current_side = SIDE_TOP;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    continue; // Skip to next iteration
                }

                // Victory celebration - Blink BLUE for Player 2 (5 seconds)
                mh_x25_set_color(light_handle, MH_X25_COLOR_DARK_BLUE);
                mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                mh_x25_set_gobo_rotation(light_handle, 0);

                for (int i = 0; i < 10; i++) // 10 blinks = 5 seconds (500ms per cycle)
                {
                    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL); // ON
                    vTaskDelay(pdMS_TO_TICKS(250));
                    mh_x25_set_dimmer(light_handle, 0); // OFF
                    vTaskDelay(pdMS_TO_TICKS(250));
                }
                mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL); // End with light ON

                // Ball stays at TOP - reset to white, same player tries again
                mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                // current_side remains SIDE_TOP - no position change needed
                vTaskDelay(pdMS_TO_TICKS(500));

                // Now wait INDEFINITELY for player to hit (no more timeouts)
                ESP_LOGI(TAG, "⬆Waiting for Player 1 to hit (no timeout)...");
                xEventGroupClearBits(paddle_events, PADDLE_TOP_HIT);
                bits = xEventGroupWaitBits(
                    paddle_events,
                    PADDLE_TOP_HIT,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY // Wait forever - no more point loss
                );

                ESP_LOGI(TAG, "Player 1 HIT! Ball moving to BOTTOM...");

                // Check if button was pressed for fireball effect
                if (last_btn_left_pressed == 0)
                {
                    ESP_LOGI(TAG, "FIREBALL ACTIVATED by Player 1!");
                    mh_x25_set_color(light_handle, MH_X25_COLOR_RED);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4);
                    mh_x25_set_gobo_rotation(light_handle, 200);
                }
                else
                {
                    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                    mh_x25_set_gobo_rotation(light_handle, 0);
                }

                // Generate random pan position for BOTTOM
                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                ESP_LOGI(TAG, "Moving to BOTTOM (pan=%d, tilt=%d)", pan_position, tilt_bottom);

                // Move ball to BOTTOM with random pan
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_bottom << 8);
                current_side = SIDE_BOTTOM;

                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
        else
        { // current_side == SIDE_BOTTOM
            // Ball is at BOTTOM, wait for RIGHT paddle (ID=2) to hit
            ESP_LOGI(TAG, "⬇Ball at BOTTOM - waiting for RIGHT paddle (ID=2)...");

            // Clear any pre-registered hits from when it wasn't this player's turn
            xEventGroupClearBits(paddle_events, PADDLE_BOTTOM_HIT);

            bits = xEventGroupWaitBits(
                paddle_events,
                PADDLE_BOTTOM_HIT,
                pdTRUE, // Clear bit after reading
                pdFALSE,
                timeout_ticks // Wait with timeout
            );

            if (bits & PADDLE_BOTTOM_HIT)
            {
                ESP_LOGI(TAG, "RIGHT PADDLE HIT! Ball moving to TOP...");

                // Check if button was pressed for fireball effect
                if (last_btn_right_pressed == 0)
                {
                    ESP_LOGI(TAG, "🔥 FIREBALL ACTIVATED by Player 2!");
                    mh_x25_set_color(light_handle, MH_X25_COLOR_RED);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4); // Fireball gobo
                    mh_x25_set_gobo_rotation(light_handle, 200);  // Fast rotation
                }
                else
                {
                    // Normal ball appearance
                    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                    mh_x25_set_gobo_rotation(light_handle, 0);
                }

                // Generate random pan position for TOP
                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                ESP_LOGI(TAG, "Moving to TOP (pan=%d, tilt=%d)", pan_position, tilt_top);

                // Move ball to TOP with random pan
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                current_side = SIDE_TOP;

                vTaskDelay(pdMS_TO_TICKS(1000)); // Movement delay
            }
            else
            {
                // Timeout! Player 2 (RIGHT/BOTTOM) missed - Player 1 scores
                game_score.score_1++;
                ESP_LOGI(TAG, "TIMEOUT! Player 2 missed. Score: Player 1 = %d, Player 2 = %d",
                         game_score.score_1, game_score.score_2);

                // Send score to clients
                esp_now_send(NULL, (uint8_t *)&game_score, sizeof(game_score_t));

                // Check for win condition
                if (game_score.score_1 >= 9)
                {
                    play_winning_animation(1);

                    // Reset game
                    game_score.score_1 = 0;
                    game_score.score_2 = 0;
                    esp_now_send(NULL, (uint8_t *)&game_score, sizeof(game_score_t));

                    // Reset ball to starting position (TOP)
                    pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                    mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                    current_side = SIDE_TOP;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    continue; // Skip to next iteration
                }

                // Victory celebration - Blink GREEN for Player 1 (5 seconds)
                mh_x25_set_color(light_handle, MH_X25_COLOR_GREEN);
                mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                mh_x25_set_gobo_rotation(light_handle, 0);

                for (int i = 0; i < 10; i++) // 10 blinks = 5 seconds (500ms per cycle)
                {
                    mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL); // ON
                    vTaskDelay(pdMS_TO_TICKS(250));
                    mh_x25_set_dimmer(light_handle, 0); // OFF
                    vTaskDelay(pdMS_TO_TICKS(250));
                }
                mh_x25_set_dimmer(light_handle, MH_X25_DIMMER_FULL); // End with light ON

                // Ball stays at BOTTOM - reset to white, same player tries again
                mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                // current_side remains SIDE_BOTTOM - no position change needed
                vTaskDelay(pdMS_TO_TICKS(500));

                // Now wait INDEFINITELY for player to hit (no more timeouts)
                ESP_LOGI(TAG, "⬇Waiting for Player 2 to hit (no timeout)...");
                xEventGroupClearBits(paddle_events, PADDLE_BOTTOM_HIT);
                bits = xEventGroupWaitBits(
                    paddle_events,
                    PADDLE_BOTTOM_HIT,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY // Wait forever - no more point loss
                );

                ESP_LOGI(TAG, "Player 2 HIT! Ball moving to TOP...");

                // Check if button was pressed for fireball effect
                if (last_btn_right_pressed == 0)
                {
                    ESP_LOGI(TAG, "🔥 FIREBALL ACTIVATED by Player 2!");
                    mh_x25_set_color(light_handle, MH_X25_COLOR_RED);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_4);
                    mh_x25_set_gobo_rotation(light_handle, 200);
                }
                else
                {
                    mh_x25_set_color(light_handle, MH_X25_COLOR_WHITE);
                    mh_x25_set_gobo(light_handle, MH_X25_GOBO_OPEN);
                    mh_x25_set_gobo_rotation(light_handle, 0);
                }

                // Generate random pan position for TOP
                pan_position = pan_min + (esp_random() % (pan_max - pan_min + 1));
                ESP_LOGI(TAG, "Moving to TOP (pan=%d, tilt=%d)", pan_position, tilt_top);

                // Move ball to TOP with random pan
                mh_x25_set_position_16bit(light_handle, pan_position << 8, tilt_top << 8);
                current_side = SIDE_TOP;

                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "DMX512 MH X25 Control Example");
    ESP_LOGI(TAG, "==============================");

    // Create event group for paddle communication
    paddle_events = xEventGroupCreate();
    if (paddle_events == NULL)
    {
        ESP_LOGE(TAG, "Failed to create event group!");
        return;
    }
    ESP_LOGI(TAG, "Event group created for paddle communication");

    // Initialize DMX
    dmx_config_t dmx_config = {
        .tx_pin = DMX_TX_PIN,
        .rx_pin = DMX_RX_PIN,
        .enable_pin = DMX_ENABLE_PIN,
        .uart_num = UART_NUM_1,
        .universe_size = 512 // Full DMX universe
    };

    esp_err_t ret = dmx_init(&dmx_config, &dmx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize DMX: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "DMX initialized successfully");

    // Initialize MH X25 light
    mh_x25_config_t light_config = {
        .dmx_handle = dmx_handle,
        .start_channel = MH_X25_START_CHANNEL};

    ret = mh_x25_init(&light_config, &light_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize MH X25: %s", esp_err_to_name(ret));
        dmx_deinit(dmx_handle);
        return;
    }
    ESP_LOGI(TAG, "MH X25 initialized at DMX address %d", MH_X25_START_CHANNEL);

    // Start continuous DMX transmission
    ret = dmx_start_transmission(dmx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start DMX transmission: %s", esp_err_to_name(ret));
        mh_x25_deinit(light_handle);
        dmx_deinit(dmx_handle);
        return;
    }
    ESP_LOGI(TAG, "DMX transmission started at 44Hz");

    // Wait a moment for DMX to stabilize
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Starting movement demos...");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "MOVEMENT CONTROL EXPLAINED:");
    ESP_LOGI(TAG, "- Pan (Channel 1): 0-255 controls horizontal rotation");
    ESP_LOGI(TAG, "- Tilt (Channel 2): 0-255 controls vertical inclination");
    ESP_LOGI(TAG, "- 128 is the center position for both");
    ESP_LOGI(TAG, "- Use mh_x25_set_position(handle, pan, tilt) to move");
    ESP_LOGI(TAG, "");

    // NOTE: ESP-NOW initialization and peer adding moved to espnow_receiver_task
    // to avoid initialization before ESP-NOW is ready

    // ESP_LOGI(TAG, "Starting DMX demo task...");
    // xTaskCreate(
    //     dmx_demo_task, // Task function
    //     "dmx_demo",    // Task name
    //     4096,          // Stack size (bytes)
    //     NULL,          // Parameters
    //     5,             // Priority
    //     NULL           // Task handle
    // );

    uint8_t mac[6];

    // Get WiFi STA (station) MAC address
    esp_wifi_get_mac(WIFI_IF_STA, mac);

    printf("ESP32-C3 MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    xTaskCreate(
        espnow_receiver_task, // Task function
        "espnow_rx",          // Task name
        4096,                 // Stack size (bytes)
        NULL,                 // Parameters
        5,                    // Priority (higher = more important)
        NULL                  // Task handle (optional)
    );

    // Create DMX controller task
    xTaskCreate(
        dmx_controller_task, // Task function
        "dmx_ctrl",          // Task name
        4096,                // Stack size (bytes)
        NULL,                // Parameters
        5,                   // Priority
        NULL                 // Task handle
    );

    ESP_LOGI(TAG, "Demo task created and running!");
}
