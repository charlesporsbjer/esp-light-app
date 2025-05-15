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
#include "driver/ledc.h"
#include "driver/gpio.h"

#include "light_control.h"  
#include "led_vars.h"

#define TAG "LIGHT_CONTROL"
#define MAX_DUTY_CYCLE ((1 << LEDC_DUTY_RES) - 1)

// Global variables for current light intensity and time
uint32_t currentRedLightIntensity = 0;
uint32_t currentSunLightIntensity = 0;
double currentRedLightIntensityDouble = 1.0;
double currentSunLightIntensityDouble = 1.0;

/**
 * @brief Initializes the LEDC (LED Controller) for controlling the lights.
 * 
 * This function configures the LEDC timer and channels for controlling the red light and sunlight.
 */
void init_ledc() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t red_channel = {
        .gpio_num = RED_LIGHT_GPIO,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_OUTPUT_RED,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&red_channel);

    ledc_channel_config_t sunlight_channel = {
        .gpio_num = SUNLIGHT_GPIO, // 19
        .speed_mode = LEDC_MODE,
        .channel = LEDC_OUTPUT_SUNLIGHT,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&sunlight_channel);
}

/**
 * @brief Sends the light intensity to the appropriate GPIO pin.
 * 
 * This function calculates the duty cycle based on the intensity percentage and updates the LEDC duty.
 * 
 * @param intensity The intensity of the light in percentage (0-100).
 * @param is_red_light True if controlling the red light, false if controlling the sunlight.
 */
void send_light_intensity(bool is_red_light) {
    const char* light_type = is_red_light ? "Red" : "Sunlight";
    uint32_t targetIntensity = is_red_light ? ledData.redLightIntensity : ledData.sunLightIntensity;
    uint32_t targetIntensityDuty = (targetIntensity * MAX_DUTY_CYCLE) / 100;
    uint32_t intensity = is_red_light ? currentRedLightIntensity : currentSunLightIntensity;

    ESP_LOGI(TAG, "%s target: %lu%% (%lu), setting to: %.2f%% (%lu)", light_type, targetIntensity, targetIntensityDuty, (intensity / (float)(MAX_DUTY_CYCLE) * 100), intensity);

    // Set and update the duty cycle
    if (is_red_light) {
        ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_RED, intensity);
        ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_RED);
    } else {
        ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_SUNLIGHT, intensity);
        ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_SUNLIGHT);
    }
}

/**
 * @brief This function turns off the lights by setting their duty cycle to 0.
 * 
 * @param is_red_light True if controlling the red light, false if controlling the sunlight.
 */
void turn_light_off(bool is_red_light) {
    if (is_red_light) {
        ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_RED, 0);
        ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_RED);
    } else {
        ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_SUNLIGHT, 0);
        ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_SUNLIGHT);
    }
}

/**
 * @brief This function turns on the lights by setting their duty cycle to the maximum value.
 * 
 * @param is_red_light True if controlling the red light, false if controlling the sunlight.
 */
void turn_light_on(bool is_red_light) {
    uint32_t max_duty = MAX_DUTY_CYCLE; // Maximum duty cycle
    if (is_red_light) {
        ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_RED, max_duty);
        ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_RED);
    } else {
        ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_SUNLIGHT, max_duty);
        ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_SUNLIGHT);
    }
}

/**
 * @brief This function adjusts the intensity of the light by a given delta value.
 * 
 * @param intensity Pointer to the intensity variable to be adjusted.
 * @param intensityDouble Pointer to the double representation of the intensity.
 * @param delta The change in intensity (positive or negative).
 * @param max The maximum allowed intensity.
 */
void adjust_intensity(uint32_t *intensity, double *intensityDouble, int delta, int max) {
    *intensityDouble *= pow(2, delta / 40.0);

    if (*intensityDouble > max) {
        *intensityDouble = max;
    } else if (*intensityDouble < 0.0) {
        *intensityDouble = 0.0;
    }

    *intensity = (uint32_t)(floor(*intensityDouble));
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

    // Calculate the maximum duty cycle based on LEDC resolution
    uint32_t max_duty = MAX_DUTY_CYCLE;

    // Scale the target intensity directly to the full LEDC resolution
    uint32_t targetRedLightDuty = (ledData.redLightIntensity * max_duty) / 100;
    uint32_t targetSunLightDuty = (ledData.sunLightIntensity * max_duty) / 100;

    // Adjust red light intensity if schedule is active
    if (strcmp(ledData.timeNowString, ledData.redLightStart) >= 0 && strcmp(ledData.timeNowString, ledData.redLightEnd) < 0) {
        if (currentRedLightIntensity < targetRedLightDuty) {
            adjust_intensity(&currentRedLightIntensity, &currentRedLightIntensityDouble, 1, max_duty);
        }
        send_light_intensity(true);
    } else if (currentRedLightIntensity > 0) {
        adjust_intensity(&currentRedLightIntensity, &currentRedLightIntensityDouble, -1, max_duty);
        send_light_intensity(true);
    }

    // Adjust sunlight intensity if schedule is active
    if (strcmp(ledData.timeNowString, ledData.sunlightStart) >= 0 && strcmp(ledData.timeNowString, ledData.sunlightEnd) < 0) {
        if (currentSunLightIntensity < targetSunLightDuty) {
            adjust_intensity(&currentSunLightIntensity, &currentSunLightIntensityDouble, 1, max_duty);
        }
        send_light_intensity(false);
    } else if (currentSunLightIntensity > 0) {
        adjust_intensity(&currentSunLightIntensity, &currentSunLightIntensityDouble, -1, max_duty);
        send_light_intensity(false);
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
    
    // if (ledData.daylightSavingsTime) {
    //     timeNow += 3600; // Add 1 hour for daylight savings time
    // }
    // daylight savings time is handled automatically by the app.

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

    init_ledc(); // Initialize LEDC for controlling lights
    send_light_intensity(true); // Set initial red light intensity to 0
    send_light_intensity(false); // Set initial sunlight intensity to 0

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