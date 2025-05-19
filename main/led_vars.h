#ifndef LED_VARS_H
#define LED_VARS_H

#include <stdint.h>


// Struct to encapsulate LED data
typedef struct {
    uint8_t sunLightIntensity;      // 0-255
    uint8_t currentSunLightIntensity; 
    char sunlightStart[6];
    char sunlightEnd[6];
    
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
} LEDData;

extern LEDData ledData; // Global instance of the LEDData struct

extern bool setup_received;


#endif // LED_VARS_H