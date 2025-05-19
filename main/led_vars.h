#ifndef LED_VARS_H
#define LED_VARS_H

#include <stdint.h>
#include <stdbool.h>


// Struct to encapsulate LED data
typedef struct {
    uint8_t lightIntensity;      // 0-255
    uint8_t currentLightIntensity; 
    char lightStart[6];
    char lightEnd[6];
    
    uint32_t timeNow;           // Unix timestamp in seconds
    char timeNowString[6];      // HH:MM representation of timeNow
    int8_t timezone;           // Time zone offset in hours

    bool monday;
    bool tuesday;
    bool wednesday;
    bool thursday;
    bool friday;
    bool saturday;
    bool sunday;
} led_data_t;

extern led_data_t ledData; // Global instance of the led_data_t struct

extern bool setup_received;


#endif // LED_VARS_H