#include "headlight_controller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

moving_head_t moving_head = {
        .pan_low = 125,      
        .tilt_low = 125,     
        .pan_tilt_speed = 0,           
        .red = 255,           
        .green = 0,
        .blue = 0,
        .color_makro = COLOR_NEUTRAL,
        .effects = EFFECT_NEUTRAL,
        .dimmer = 255,        
        .pan_high = 0,   
        .tilt_high = 0  
};

moving_head_t* get_data(void){
    return &moving_head;
}

// Funktion zur Generierung der DMX-Daten
void generate_dmx_data(const moving_head_t* config, uint8_t* dmx_data) {
    
    // Kanal 1: PAN 8-Bit
    dmx_data[0] = config->pan_low;
    
    // Kanal 2: TILT 8-Bit
    dmx_data[1] = config->tilt_low;
    
    // Kanal 3: Geschwindigkeit
    dmx_data[2] = config->pan_tilt_speed;
    
    // Kanal 4: Rot
    dmx_data[3] = config->red;
    
    // Kanal 5: Grün
    dmx_data[4] = config->green;
    
    // Kanal 6: Blau
    dmx_data[5] = config->blue;
    
    // Kanal 7: Farbmakro
    dmx_data[6] = config->color_makro;
    
    // Kanal 8: Effekte
    dmx_data[7] = config->effects;
    
    // Kanal 9: Dimmer
    dmx_data[8] = config->dimmer;


    // Kanal 10: PAN 16-Bit (High-Byte, Low-Byte)
    // (pan & 0xff00) >> 8
    dmx_data[9] = config->pan_high;  // High-Byte
    dmx_data[10] = config->tilt_high;        // Low-Byte   
}

void set_position(moving_head_t* head, uint8_t pan, uint8_t tilt) {
    head->pan_low = pan & 0xff;
    head->tilt_low = tilt & 0xff;
    // head->pan_high = (pan & 0xff00) >> 8;
    // head->tilt_high = (tilt & 0xff00) >> 8;
}

void set_position_pan(moving_head_t* head, uint8_t pan){
    head->pan_low = pan & 0xff;
    // head->pan_high = (pan & 0xff00) >> 8;
}

void set_position_tilt(moving_head_t* head, uint8_t tilt){
    head->tilt_low = tilt & 0xff;
    // head->tilt_high = (tilt & 0xff00) >> 8;
}

void set_rgb_color(moving_head_t* head, uint8_t red, uint8_t green, uint8_t blue) {
    head->red = red;
    head->green = green;
    head->blue = blue;
}

void set_color_macro(moving_head_t* head, color_macro_t macro) {
    head->color_makro = macro;
}

void set_effect(moving_head_t* head, effect_t effect) {
    head->effects = effect;
}

void set_dimmer(moving_head_t* head, uint8_t intensity) {
    head->dimmer = intensity;
}

void set_speed(moving_head_t* head, uint8_t speed) {
    head->pan_tilt_speed = speed;
}
