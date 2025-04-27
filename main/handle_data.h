#ifndef HANDLE_DATA_H
#define HANDLE_DATA_H

#include "shared_queue.h"

/**
 * @brief Initializes the global variables for LED data.
 */
void init_led_data();

/**
 * @brief Parses the JSON string to get timeNow, days, schedule, LEDs, and values.
 * 
 * @param jsonstr The JSON string to parse.
 * @return 0 on success, -1 on failure.
 */
int parse_led_data(char* jsonstr);

/**
 * @brief Prints the global variables for LED data.
 */
void print_led_data();

/**
 * @brief Handles the light protocol by parsing the JSON string and storing the values in a global struct.
 * 
 * @param data_received Struct with JSON string containing the LED data and length.
 */
void handle_light_protocol(QueueData_t *data_received);

/**
 * @brief Task to control the lights.
 * 
 * @param pvParameters Parameters for the task (not used).
 */
void handle_data_task(void *pvParameters);


#endif // HANDLE_DATA_H