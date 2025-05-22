#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h> // Include math library for pow function

#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"


#include "light_control.h"  
#include "led_vars.h"
#include "dali.h"
#include "logger.h"
#include "pulse.h"

#define TAG "LIGHT_CONTROL"

#define PULSE // Uncomment to disable pulse at the end of the schedule

#define DALI_RX_PIN GPIO_NUM_4 //Not used
#define DALI_TX_PIN GPIO_NUM_5



/**
 * @brief Sends a simplified DALI command to short address 1.
 * 
 * Wraps dali_transaction() and uses presets since we don't need much of the functionality.
 * 
 * @param command The DALI command to send.
 */
void send_dali_command(const uint8_t command) {
    esp_err_t status = dali_transaction(DALI_ADDRESS_TYPE_SHORT, 1, true, command, false, DALI_TX_TIMEOUT_DEFAULT_MS, NULL);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "DALI transaction failed: %s", esp_err_to_name(status));
    }
}

/**
 * @brief Flash the light once.
 * 
 * This function flashes the light once by sending recall max followed by recall min, then turning it off.
 */
void flash_light() {
    send_dali_command(RECALL_MAX);
    vTaskDelay(pdMS_TO_TICKS(50)); // Delay in milliseconds
    send_dali_command(RECALL_MIN);
    vTaskDelay(pdMS_TO_TICKS(2)); // Delay in milliseconds
    send_dali_command(OFF); // Turn off the light
}

/**
 * @brief Pulses the light for some duration.
 * 
 * This function pulses the light from 1 to 254 quickly for a duration and then turns it off.
 */
#ifdef PULSE
void pulse_light(uint8_t count) {
    while (count--) {
        while (ledData.currentLightIntensity < MAX_BRIGHTNESS) {
            send_dali_command(ON_AND_STEP_UP);
            vTaskDelay(pdMS_TO_TICKS(2)); // Delay in milliseconds
            ledData.currentLightIntensity++;
        }
        while (ledData.currentLightIntensity > MIN_BRIGHTNESS) {
            send_dali_command(STEP_DOWN);
            vTaskDelay(pdMS_TO_TICKS(2)); // Delay in milliseconds
            ledData.currentLightIntensity--;
        }
    }
    send_dali_command(OFF); // Turn off the light after pulsing
}
#endif //PULSE

/**
 * @brief Sends dimmer data according to the schedule.
 * 
 * This function adjusts the light intensities based on the current time and predefined schedules.
 */
void send_dimmer_data() {
    // Convert timeNow to a string
    update_timeNowString(ledData.timeNow, ledData.timeNowString);

    bool within_schedule = 
        strcmp(ledData.timeNowString, ledData.lightStart) >= 0 &&
        strcmp(ledData.timeNowString, ledData.lightEnd) < 0;
    
    scheduleStatus.scheduleIsActive = within_schedule;
    
    if (within_schedule) {

        if (ledData.currentLightIntensity < ledData.lightIntensity) {
            send_dali_command(ON_AND_STEP_UP);
            ledData.currentLightIntensity++;
        }
        return;
    }
    
    // Outside scheduled window: dim down or turn off
    if (ledData.currentLightIntensity > MIN_BRIGHTNESS) {
        ledData.currentLightIntensity--;
        send_dali_command(STEP_DOWN);

        if (ledData.currentLightIntensity == MIN_BRIGHTNESS) {
            send_dali_command(OFF);
#ifdef PULSE
            pulse_light(3); 
            //xTaskCreate(pulse_task, "pulse_task", PULSE_STACK_SIZE, NULL, PULSE_TASK_PRIORITY, NULL); // Create a new task for pulsing
#endif
        }
    }
}

/**
 * @brief Continuously updates the current time (timeNow).
 * 
 * This function increments the current time based on the elapsed time since the last update.
 */
void update_time_now() {
    static int64_t last_time_us = 0;
    int64_t current_time_us = esp_timer_get_time(); // Get time in microseconds

#ifdef DEBUG
    ESP_LOGI(TAG, "Last time: %lld us", last_time_us);
    ESP_LOGI(TAG, "Current time: %lld us", current_time_us);
#endif

    // Initialize on first call
    if (last_time_us == 0) {
        last_time_us = current_time_us;
        return;
    }

    int64_t delta_us = current_time_us - last_time_us;

    // Only proceed if >= 1 second has passed
    if (delta_us < 1000000) {
        return;
    }

    // Update last_time_us and timeNow accordingly
    uint32_t elapsed_seconds = delta_us / 1000000;
    last_time_us += (int64_t)elapsed_seconds * 1000000; // keep residual microseconds

    ledData.timeNow += elapsed_seconds;

#ifdef DEBUG
    ESP_LOGI(TAG, "Elapsed seconds: %u", elapsed_seconds);
    ESP_LOGI(TAG, "Updated timeNow: %lu", ledData.timeNow);
#endif
}

/**
 * @brief Converts a Unix timestamp to the corresponding day of the week.
 * 
 * This function uses the gmtime_r function to convert the timestamp to a struct tm and then retrieves the day of the week.
 * It also updates the logger with the current day of the week.
 * 
 * @return The day of the week as a string (e.g., "Monday", "Tuesday").
 */
const char* get_day_of_week() {
    // Convert the timestamp to a time_t type
    time_t raw_time = (time_t)ledData.timeNow;

    // Convert the time_t to a struct tm
    struct tm time_info;
    gmtime_r(&raw_time, &time_info); // Use gmtime_r for thread safety

    // Array of day names
    static const char* days_of_week[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };

    // Update logger with the current day of the week
    logger_set_day_of_week(days_of_week[time_info.tm_wday]);

    // Return the day of the week
    return days_of_week[time_info.tm_wday];
}

/**
 * @brief Checks if the schedule is active for the current day.
 * 
 * This function compares the current day of the week with the schedule and returns true if active.
 * 
 * @return true if the schedule is active for the current day, false otherwise.
 */
bool is_day_active(void) {
    static const char* days[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };

    const char* day_of_week = get_day_of_week();
    bool active_flags[] = {
        ledData.sunday,
        ledData.monday,
        ledData.tuesday,
        ledData.wednesday,
        ledData.thursday,
        ledData.friday,
        ledData.saturday
    };

    for (int i = 0; i < 7; i++) {
        if (strcmp(day_of_week, days[i]) == 0) {
#ifdef DEBUG
            ESP_LOGI(TAG, "Today is %s, Active: %d", days[i], active_flags[i]);
#endif
            return active_flags[i];
        }
    }

    // If day string is unexpected (shouldn't happen), assume inactive
    ESP_LOGW(TAG, "Unknown day: %s", day_of_week);
    return false;
}

/**
 * @brief Converts unix timestamp to HH:MM format.
 * 
 * This function formats the Unix timestamp into a human-readable string, considering timezone.
 * 
 * @param timeNow The unix timestamp in seconds.
 * @param buffer The buffer to store the converted string.
 */
void update_timeNowString(uint32_t timeNow, char* buffer) {
    // Convert timeNow to HH:MM format
    timeNow += ledData.timezone * 3600; // Add timezone offset in seconds

    uint32_t hours = (timeNow / 3600) % 24;
    uint32_t minutes = (timeNow / 60) % 60;

    snprintf(buffer, 6, "%02lu:%02lu", hours, minutes);
}

/**
 * @brief Task to control the lights.
 * 
 * This task periodically updates the light intensities according to schedule.
 * 
 * @param pvParameters Parameters for the task (not used).
 */
void light_control_task(void *pvParameters) {
#ifdef DEBUG
    ESP_LOGI(TAG, "Light control task started");
#endif // DEBUG
    static uint16_t log_counter = 0; // Counter to control log frequency
    logger_init_schedule_status(); // Initialize schedule status

    dali_init(DALI_RX_PIN, DALI_TX_PIN);

    // SEND ONE PULSE, THEN OFF
    while (!setup_received) {
        ESP_LOGI(TAG, "No setup sent, sending blink");
        send_dali_command(ON_AND_STEP_UP);
        send_dali_command(STEP_DOWN);
        send_dali_command(OFF);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    while (1) {
        if (setup_received) {
            update_time_now(); // Update the current time

            if (is_day_active()) {
                scheduleStatus.todayIsActive = true; // Set today as active
                send_dimmer_data(); // Send dimmer data according to the schedule
            }
            else { 
                scheduleStatus.todayIsActive = false; // Set today as inactive
            }

            if (log_counter % 10 == 0) { // Print every 50 iterations
               logger_print_schedule_status(&ledData); // Print schedule status
            }
            log_counter++;
        }
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
    vTaskDelete(NULL); // Delete the task when done
}