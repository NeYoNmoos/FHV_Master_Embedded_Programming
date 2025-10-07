/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char* TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

// Game of Life constants
#define GRID_SIZE 5
#define TOTAL_LEDS 25

static uint8_t s_led_state = 0;

// Game of Life grid (current and next generation)
static uint8_t current_grid[GRID_SIZE][GRID_SIZE];
static uint8_t next_grid[GRID_SIZE][GRID_SIZE];

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;


static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 25, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

// Game of Life Functions

/* Initialize the grid with a "Glider" pattern - a classic Game of Life pattern */
static void init_game_of_life(void)
{
    // Clear the grid first
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            current_grid[row][col] = 0;
        }
    }

    current_grid[0][1] = 1;  // Row 0, Col 1
    current_grid[1][2] = 1;  // Row 1, Col 2
    current_grid[2][0] = 1;  // Row 2, Col 0
    current_grid[2][1] = 1;  // Row 2, Col 1
    current_grid[2][2] = 1;  // Row 2, Col 2

    ESP_LOGI(TAG, "Game of Life initialized with Glider pattern");
}

/* Count living neighbors for a cell */
static int count_neighbors(int row, int col)
{
    int count = 0;

    // Check all 8 neighbors (with boundary checking)
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue; // Skip the cell itself

            int new_row = row + dr;
            int new_col = col + dc;

            // Check boundaries
            if (new_row >= 0 && new_row < GRID_SIZE &&
                new_col >= 0 && new_col < GRID_SIZE) {
                count += current_grid[new_row][new_col];
            }
        }
    }

    return count;
}

/* Apply Game of Life rules and calculate next generation */
static void calculate_next_generation(void)
{
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            int neighbors = count_neighbors(row, col);
            uint8_t current_cell = current_grid[row][col];

            // Conway's Game of Life rules:
            if (current_cell == 1) {
                // Living cell
                if (neighbors < 2) {
                    next_grid[row][col] = 0; // Dies from underpopulation
                }
                else if (neighbors == 2 || neighbors == 3) {
                    next_grid[row][col] = 1; // Survives
                }
                else {
                    next_grid[row][col] = 0; // Dies from overpopulation
                }
            }
            else {
                // Dead cell
                if (neighbors == 3) {
                    next_grid[row][col] = 1; // Becomes alive by reproduction
                }
                else {
                    next_grid[row][col] = 0; // Stays dead
                }
            }
        }
    }

    // Copy next generation to current
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            current_grid[row][col] = next_grid[row][col];
        }
    }
}

/* Convert 2D grid position to LED strip index */
static int grid_to_led_index(int row, int col)
{
    return row * GRID_SIZE + col;
}

/* Display the current grid on the LED strip */
static void display_grid(void)
{
#ifdef CONFIG_BLINK_LED_STRIP
    led_strip_clear(led_strip);

    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            int led_index = grid_to_led_index(row, col);

            if (current_grid[row][col] == 1) {
                // Living cell - green
                led_strip_set_pixel(led_strip, led_index, 20, 0, 20);
            }
            // Dead cells remain off (black)
        }
    }

    led_strip_refresh(led_strip);
#endif
}

void app_main(void)
{
    /* Configure the peripheral according to the LED type */
    configure_led();

    init_game_of_life();

    ESP_LOGI(TAG, "Starting Conway's Game of Life on 5x5 LED grid!");
    ESP_LOGI(TAG, "Press Ctrl+C in monitor to stop");

    int generation = 0;

    while (1)
    {
        ESP_LOGI(TAG, "=== Generation %d ===", generation);


        // Display on LED strip
        display_grid();

        // Wait before next generation
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay

        // Calculate and apply next generation
        calculate_next_generation();

        generation++;

        // Reset after 20 generations to see the pattern repeat
        if (generation >= 15) {
            ESP_LOGI(TAG, "Restarting with fresh Glider pattern...");
            init_game_of_life();
            generation = 0;
            vTaskDelay(2000 / portTICK_PERIOD_MS); // 2 second pause
        }
    }
}
