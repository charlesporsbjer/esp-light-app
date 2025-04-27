#ifndef LED_VARS_H
#define LED_VARS_H

#include <stdint.h>


// Define GPIO pins for red light and sunlight
#define RED_LIGHT_GPIO 18
#define SUNLIGHT_GPIO 19

// Define LEDC channel configurations
#define LEDC_TIMER              LEDC_TIMER_0
#if defined(LEDC_HIGH_SPEED_MODE)
    #define LEDC_MODE           LEDC_HIGH_SPEED_MODE
#else
    #define LEDC_MODE           LEDC_LOW_SPEED_MODE
#endif
#define LEDC_OUTPUT_RED         LEDC_CHANNEL_0
#define LEDC_OUTPUT_SUNLIGHT    LEDC_CHANNEL_1
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_FREQUENCY          5000             // Frequency in Hertz

// Struct to encapsulate LED data
typedef struct {
    int redLightIntensity;      // In percentage
    char redLightStart[6];      // HH:MM + null terminator
    char redLightEnd[6];
    int sunLightIntensity;      // In percentage
    char sunlightStart[6];
    char sunlightEnd[6];
    
    uint32_t timeNow;           // Unix timestamp in seconds
    char timeNowString[6];      // HH:MM representation of timeNow
    bool daylightSavingsTime;   // True if DST is active, false otherwise
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