
#include "pulse.h"
#include "led_vars.h"     
#include "light_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



#define PULSE_DELAY_MS 2

typedef enum {
    PULSE_UP,
    PULSE_DOWN
} PulseState_t;

typedef struct {
    PulseState_t state;
    uint8_t pulses_remaining;
    TickType_t last_tick;
} PulseController_t;

/**
 * @brief Task to pulse the light.
 * 
 * This task gradually increases and decreases the light intensity in a pulsing manner.
 * 
 * @param pvParameters Parameters for the task (not used).
 */
void pulse_task(void *pvParameters) {
    PulseController_t controller = {
        .state = PULSE_UP, 
        .pulses_remaining = 5,
        .last_tick = xTaskGetTickCount()
    };
    
    ledData.currentLightIntensity = MIN_BRIGHTNESS; // Start from minimum brightness

    while (1) {
        TickType_t now = xTaskGetTickCount();

        if ((TickType_t)(now - controller.last_tick) < pdMS_TO_TICKS(PULSE_DELAY_MS)) {
            vTaskDelay(pdMS_TO_TICKS(1)); // Yield to other tasks
            continue;
        }
        controller.last_tick = now;
        switch (controller.state) {
            case PULSE_UP:
                if (ledData.currentLightIntensity < MAX_BRIGHTNESS) {
                    send_dali_command(ON_AND_STEP_UP);
                    ledData.currentLightIntensity++;
                } else {
                    controller.state = PULSE_DOWN;
                }
                break;

            case PULSE_DOWN:
                if (ledData.currentLightIntensity > MIN_BRIGHTNESS) {
                    send_dali_command(STEP_DOWN);
                    ledData.currentLightIntensity--;
                } else {
                    controller.pulses_remaining--;
                    if (controller.pulses_remaining == 0) {
                        send_dali_command(OFF);
                        controller.state = PULSE_UP; // Reset to start
                        vTaskDelete(NULL); // Delete the task when done
                    } else {
                        controller.state = PULSE_UP;
                    }
                }
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}