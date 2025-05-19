#ifndef LIGHT_CONTROL_H
#define LIGHT_CONTROL_H

#include <stdint.h>

/**
 * @brief Interface for dali commands. 
 * 
 * This function simplifies sending dali commands since we don't need much of the functionality.
 * 
 * @param command command to be sent.
 */
void send_dali_command(char* command);

/**
 * @brief Sends dimmer data according to the schedule.
 * 
 * This function adjusts the light intensities based on the current time and predefined schedules.
 * 
 * @return int 0 on success, -1 on failure.
 */
int send_dimmer_data();

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
char* get_day_of_week(uint32_t timestamp);

/**
 * @brief Checks if the schedule is active for the current day.
 * 
 * This function compares the current day of the week with the schedule and returns true if active.
 * 
 * @return true if the schedule is active for the current day, false otherwise.
 */
bool is_day_active();

/**
 * @brief Converts the timeNow to a string in HH:MM format.
 * 
 * This function formats the Unix timestamp into a human-readable time string, considering timezone and DST.
 * 
 * @param timeNow The unix timestamp in seconds.
 * @param buffer The buffer to store the converted string.
 */
void convert_timeNow_to_string(uint32_t timeNow, char* buffer);

/**
 * @brief Task to control the lights.
 * 
 * This task periodically updates the light intensities and processes incoming data.
 * 
 * @param pvParameters Parameters for the task (not used).
 */
void light_control_task(void *pvParameters);

#endif // LIGHT_CONTROL_H