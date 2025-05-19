#ifndef LIGHT_CONTROL_H
#define LIGHT_CONTROL_H

#include <stdint.h>
#include "dali_commands.h"

// Shortened DALI commands, more can be found in dali_commands.h
#define ON_AND_STEP_UP  DALI_COMMAND_ON_AND_STEP_UP
#define STEP_DOWN       DALI_COMMAND_STEP_DOWN
#define OFF             DALI_COMMAND_OFF
#define RECALL_MIN      DALI_COMMAND_RECALL_MIN_LEVEL
#define RECALL_MAX      DALI_COMMAND_RECALL_MAX_LEVEL

#define MIN_BRIGHTNESS 1
#define MAX_BRIGHTNESS 254

/**
 * @brief Interface for dali commands. 
 * 
 * Wraps dali_transaction() and uses presets since we don't need much of the functionality.
 * 
 * @param command command to be sent.
 */
void send_dali_command(uint8_t command);

/**
 * @brief Flash the light once.
 * 
 * This function flashes the light once by sending recall max followed by recall min, then turning it off.
 */
void flash_light();

/**
 * @brief Pulses the light for some duration.
 * 
 * This function pulses the light from 1 to 255 quickly for a duration and then turns it off.
 */
void pulse_light();

/**
 * @brief Sends dimmer data according to the schedule.
 * 
 * This function adjusts the light intensities based on the current time and predefined schedules.
 * 
 * @return int 0 on success, -1 on failure.
 */
void send_dimmer_data();

/**
 * @brief Continuously updates the current time (timeNow).
 * 
 * This function increments the current time based on the elapsed time since the last update.
 */
void update_time_now();

/**
 * @brief Converts a Unix timestamp to the corresponding day of the week.
 * 
 * @param timestamp The Unix timestamp in seconds.
 * @return The day of the week as a string (e.g., "Monday", "Tuesday").
 */
const char* get_day_of_week();

/**
 * @brief Checks if the schedule is active for the current day.
 * 
 * This function compares the current day of the week with the schedule and returns true if active.
 * 
 * @return true if the schedule is active for the current day, false otherwise.
 */
bool is_day_active();

/**
 * @brief Converts unix timestamp to HH:MM format.
 * 
 * This function formats the Unix timestamp into a human-readable string, considering timezone.
 * 
 * @param timeNow The unix timestamp in seconds.
 * @param buffer The buffer to store the converted string.
 */
void update_timeNowString(uint32_t timeNow, char* buffer);

/**
 * @brief Task to control the lights.
 * 
 * This task periodically updates the light intensities according to schedule.
 * 
 * @param pvParameters Parameters for the task (not used).
 */
void light_control_task(void *pvParameters);

#endif // LIGHT_CONTROL_H