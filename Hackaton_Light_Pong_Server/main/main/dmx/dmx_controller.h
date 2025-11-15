#ifndef DMX_CONTROLLER_H
#define DMX_CONTROLLER_H

#include <stdint.h>
#include <stddef.h>
#include "sdkconfig.h"

// Konfiguration aus Kconfig oder Fallback-Werte
// #if CONFIG_DMX_ENABLED
// #define DMX_UART_NUM       CONFIG_DMX_UART
// #define DE_PIN            CONFIG_DMX_DE_PIN
// #define DMX_TX_PIN        CONFIG_DMX_UART_TX
// #define DMX_RX_PIN        CONFIG_DMX_UART_RX
// #define DMX_BAUDRATE      CONFIG_DMX_UART_BAUDRATE
// #define DMX_CHANNELS      CONFIG_DMX_CHANNELS
// #else
#define DMX_UART_NUM      UART_NUM_0
#define DE_PIN            9
#define DMX_TX_PIN        21
#define DMX_RX_PIN        20
#define DMX_BAUDRATE      250000
#define DMX_CHANNELS      512
// #endif

extern uint8_t dmxData[DMX_CHANNELS];

/**
 * @brief Initialisiert den DMX-Treiber
 * 
 * Konfiguriert UART und GPIO-Pins für DMX512-Protokoll
 */
void dmx_init(void);

/**
 * @brief Sendet einen DMX-Frame
 * 
 * @param data Pointer zu den DMX-Daten
 * @param length Anzahl der zu sendenden Kanäle
 * 
 * Sendet einen kompletten DMX-Frame mit Break, MAB, Start-Code und Daten
 */
void send_dmx_frame(const uint8_t *data, size_t length);

/**
 * @brief Setzt alle DMX-Kanäle auf 0
 */
void dmx_clear_all(void);

#endif