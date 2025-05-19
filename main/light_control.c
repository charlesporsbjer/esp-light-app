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
#include "dali_commands.h"

#define TAG "LIGHT_CONTROL"

#define ON_AND_STEP_UP  DALI_COMMAND_ON_AND_STEP_UP
#define STEP_UP         DALI_COMMAND_STEP_UP
#define STEP_DOWN       DALI_COMMAND_STEP_DOWN
#define OFF             DALI_COMMAND_OFF

int result = 0; // For storing result of dali query presumably, seems redundant. 

/**
 * @brief Interface for dali commands. 
 * 
 * This function simplifies sending dali commands since we don't need much of the functionality.
 * 
 * @param command command to be sent.
 */
void send_dali_command(uint8_t command) {
    esp_err_t status = dali_transaction(DALI_ADDRESS_TYPE_SHORT, 1, true, command, false, DALI_TX_TIMEOUT_DEFAULT_MS, &result);
    if (status == ESP_OK) {
        // Command successfully sent and response received
    } else {
        // Handle error
        ESP_LOGE(TAG, "DALI transaction failed: %s", esp_err_to_name(status));
    }
}

/**
 * @brief Sends dimmer data according to the schedule.
 * 
 * This function adjusts the light intensities based on the current time and predefined schedules.
 * 
 * @return int 0 on success, -1 on failure.
 */
int send_dimmer_data() {
    // Convert timeNow to a string
    convert_timeNow_to_string(ledData.timeNow, ledData.timeNowString);

    // Adjust sunlight intensity if schedule is active
    if (strcmp(ledData.timeNowString, ledData.sunlightStart) >= 0 && strcmp(ledData.timeNowString, ledData.sunlightEnd) < 0) {
        if (ledData.currentSunLightIntensity < ledData.sunLightIntensity) {
            send_dali_command(ON_AND_STEP_UP);
            ledData.currentSunLightIntensity++;
        }
    } else if (ledData.currentSunLightIntensity > 0) {
        send_dali_command(STEP_DOWN);
        ledData.currentSunLightIntensity--;
    }

    return 0;
}

/**
 * @brief Continuously updates the current time (timeNow).
 * 
 * This function increments the current time based on the elapsed time since the last update.
 */
void update_time_now() {
    static int64_t elapsed_time_us = 0;
    static int64_t last_time_us = 0;
    int64_t elapsed_time_s = 0;
    int64_t current_time_us = esp_timer_get_time(); // Get time in microseconds

#ifdef DEBUG
    ESP_LOGI(TAG, "Last time: %lld us", last_time_us);
    ESP_LOGI(TAG, "Current time: %lld us", current_time_us);
#endif

    if (last_time_us == 0) {
        last_time_us = current_time_us; // Initialize last_time_us on the first call
        return;
    }
    
    // Update elapsed_time_us
    elapsed_time_us += current_time_us - last_time_us;
    
#ifdef DEBUG
    ESP_LOGI(TAG, "Elapsed time: %lld us", elapsed_time_us);
#endif
    
    // Update last_time_us before returning
    last_time_us = current_time_us;
    
    if (elapsed_time_us < 1000000) {
        return; // No need to update if less than 1 second has passed
    }
    
    // Calculate elapsed seconds and update timeNow
    elapsed_time_s = elapsed_time_us / 1000000;
    elapsed_time_us -= elapsed_time_s * 1000000;
    ledData.timeNow += (uint32_t)elapsed_time_s;
    
    // Debug log
#ifdef DEBUG
    ESP_LOGI(TAG, "Updated timeNow: %lu", ledData.timeNow); 
#endif
}

/**
 * @brief Converts a Unix timestamp to the corresponding day of the week.
 * 
 * @param timestamp The Unix timestamp in seconds.
 * @return The day of the week as a string (e.g., "Monday", "Tuesday").
 */
char* get_day_of_week(uint32_t timestamp) {
    // Convert the timestamp to a time_t type
    time_t raw_time = (time_t)timestamp;

    // Convert the time_t to a struct tm
    struct tm time_info;
    gmtime_r(&raw_time, &time_info); // Use gmtime_r for thread safety

    // Array of day names
    char* days_of_week[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };

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
bool is_day_active() {
    // Check if the schedule is active for the current day
    char* day_of_week = get_day_of_week(ledData.timeNow);

#ifdef DEBUG
    ESP_LOGI(TAG, "Calculated day of the week: %s", day_of_week);
    ESP_LOGI(TAG, "Monday: %d, Tuesday: %d, Wednesday: %d, Thursday: %d, Friday: %d, Saturday: %d, Sunday: %d",
             ledData.monday, ledData.tuesday, ledData.wednesday, ledData.thursday, ledData.friday, ledData.saturday, ledData.sunday);
#endif // DEBUG

    if ((strcmp(day_of_week, "Monday") == 0 && !ledData.monday) ||
        (strcmp(day_of_week, "Tuesday") == 0 && !ledData.tuesday) ||
        (strcmp(day_of_week, "Wednesday") == 0 && !ledData.wednesday) ||
        (strcmp(day_of_week, "Thursday") == 0 && !ledData.thursday) ||
        (strcmp(day_of_week, "Friday") == 0 && !ledData.friday) ||
        (strcmp(day_of_week, "Saturday") == 0 && !ledData.saturday) ||
        (strcmp(day_of_week, "Sunday") == 0 && !ledData.sunday)) 
    {
        ESP_LOGI(TAG, "Schedule is not active for %s", day_of_week);
        return false; // Schedule is not active for the current day
    }

    return true; // Schedule is active for the current day
}

/**
 * @brief Converts the timeNow to a string in HH:MM format.
 * 
 * This function formats the Unix timestamp into a human-readable time string, considering timezone and DST.
 * 
 * @param timeNow The unix timestamp in seconds.
 * @param buffer The buffer to store the converted string.
 */
void convert_timeNow_to_string(uint32_t timeNow, char* buffer) {
    // Convert timeNow to HH:MM format
    timeNow += ledData.timezone * 3600; // Add timezone offset in seconds

    uint32_t hours = (timeNow / 3600) % 24;
    uint32_t minutes = (timeNow / 60) % 60;

    snprintf(buffer, 6, "%02lu:%02lu", hours, minutes);
}

/**
 * @brief Task to control the lights.
 * 
 * This task periodically updates the light intensities and processes incoming data.
 * 
 * @param pvParameters Parameters for the task (not used).
 */
void light_control_task(void *pvParameters) {
    static int log_counter = 0; // Counter to control log frequency

    dali_init(GPIO_NUM_21, GPIO_NUM_22);

    // SEND ONE PULSE MAYBE, THEN OFF
    send_dali_command("STEP_UP")

    while (1) {
        if (setup_received) {
            if (is_day_active()) {
                send_dimmer_data(); // Send dimmer data according to the schedule
            }

            // Print time now string less often
            if (log_counter % 10 == 0) { // Print every 10 iterations
                ESP_LOGI(TAG, "Day now: %s", get_day_of_week(ledData.timeNow));
                ESP_LOGI(TAG, "Time now string: %s, UTS: %lu", ledData.timeNowString, ledData.timeNow);
            }
            log_counter++;

            update_time_now(); // Update the current time
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 1 second
    }
    vTaskDelete(NULL); // Delete the task when done
}