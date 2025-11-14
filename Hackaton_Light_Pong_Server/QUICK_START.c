/*
 * QUICK START GUIDE - DMX MH X25 Control
 * =======================================
 *
 * STEP 1: VERIFY YOUR HARDWARE CONNECTIONS
 * -----------------------------------------
 * Check your Clownfish ESP32-C3 board schematic for RS-485 pins!
 *
 * Typical connections:
 * - ESP32 TX    → RS-485 DI (Data Input)
 * - ESP32 GPIO  → RS-485 DE/RE (Direction Enable)
 * - RS-485 A    → DMX Pin 3 (DMX+)
 * - RS-485 B    → DMX Pin 2 (DMX-)
 * - DMX Pin 1   → Ground
 *
 * STEP 2: UPDATE GPIO PINS IN blink_example_main.c
 * -------------------------------------------------
 * Find these lines and update with YOUR board's actual pins:
 *
 *   #define DMX_TX_PIN          GPIO_NUM_21
 *   #define DMX_RX_PIN          GPIO_NUM_20
 *   #define DMX_ENABLE_PIN      GPIO_NUM_19
 *
 * STEP 3: CONFIGURE MH X25 LIGHT
 * -------------------------------
 * 1. Power on the MH X25
 * 2. Use the menu to set:
 *    - DMX Address: 1 (or update MH_X25_START_CHANNEL in code)
 *    - DMX Mode: 6-channel mode
 * 3. Verify the light is ready (usually displays address on screen)
 *
 * STEP 4: BUILD AND FLASH
 * -----------------------
 * $ idf.py build
 * $ idf.py -p /dev/ttyACM0 flash monitor
 *
 * STEP 5: CUSTOMIZE (OPTIONAL)
 * -----------------------------
 * Simple color control example:
 */

#include "dmx.h"
#include "mh_x25.h"

void simple_light_control_example(void)
{
    // 1. Initialize DMX
    dmx_config_t dmx_config = {
        .tx_pin = GPIO_NUM_21,
        .rx_pin = GPIO_NUM_20,
        .enable_pin = GPIO_NUM_19,
        .uart_num = UART_NUM_1,
        .universe_size = 512};
    dmx_handle_t dmx;
    dmx_init(&dmx_config, &dmx);

    // 2. Initialize Light at DMX address 1
    mh_x25_config_t light_config = {
        .dmx_handle = dmx,
        .start_channel = 1};
    mh_x25_handle_t light;
    mh_x25_init(&light_config, &light);

    // 3. Start DMX transmission
    dmx_start_transmission(dmx);

    // 4. Control the light!

    // Set a bright red color
    mh_x25_set_preset_color(light, &MH_X25_COLOR_RED);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Custom color (Purple-ish)
    mh_x25_set_dimmer(light, 255);
    mh_x25_set_rgb(light, 200, 50, 200);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Add strobe effect
    mh_x25_set_strobe(light, MH_X25_STROBE_MEDIUM);
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Turn off
    mh_x25_off(light);
}

/*
 * CONTROLLING MULTIPLE LIGHTS
 * ----------------------------
 * Set each light to a different DMX address, then create multiple handles:
 */

void multiple_lights_example(void)
{
    dmx_handle_t dmx;
    // ... initialize dmx ...

    // Light 1 at address 1 (channels 1-6)
    mh_x25_config_t light1_config = {
        .dmx_handle = dmx,
        .start_channel = 1};
    mh_x25_handle_t light1;
    mh_x25_init(&light1_config, &light1);

    // Light 2 at address 7 (channels 7-12)
    mh_x25_config_t light2_config = {
        .dmx_handle = dmx,
        .start_channel = 7};
    mh_x25_handle_t light2;
    mh_x25_init(&light2_config, &light2);

    dmx_start_transmission(dmx);

    // Control independently
    mh_x25_set_preset_color(light1, &MH_X25_COLOR_RED);
    mh_x25_set_preset_color(light2, &MH_X25_COLOR_BLUE);
}

/*
 * COMMON PIN CONFIGURATIONS FOR CLOWNFISH ESP32-C3
 * ------------------------------------------------
 * Check YOUR board schematic! Common configurations might include:
 *
 * Option 1 (UART1 on standard pins):
 *   TX: GPIO_NUM_21
 *   EN: GPIO_NUM_19
 *
 * Option 2 (Alternative UART1 pins):
 *   TX: GPIO_NUM_4
 *   EN: GPIO_NUM_5
 *
 * Option 3 (If using UART0 - NOT recommended, conflicts with console):
 *   TX: GPIO_NUM_21
 *   EN: GPIO_NUM_20
 *
 * TROUBLESHOOTING TIPS
 * --------------------
 * 1. No light response?
 *    - Check DMX address matches code
 *    - Verify RS-485 A/B wiring (try swapping if needed)
 *    - Add 120Ω termination resistor between A and B
 *
 * 2. Flickering?
 *    - Check enable pin is correct
 *    - Verify power supply is stable
 *    - Check for loose connections
 *
 * 3. Build errors?
 *    - Ensure ESP-IDF environment is activated
 *    - Run: idf.py clean && idf.py build
 *
 * 4. Serial monitor shows errors?
 *    - Check UART number doesn't conflict (use UART_NUM_1)
 *    - Verify GPIO pins are valid for your chip
 */
