#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "cJSON.h"


#include "handle_data.h"
#include "shared_queue.h"   // Include the shared queue header
#include "led_vars.h"      // Include the LED variables header
#include "light_control.h" // Include the light control header

#define TAG "HANDLE_DATA"

#define LIGHT_CONTROL_STACK_SIZE 2048 // Stack size for light control task
#define LIGHT_CONTROL_TASK_PRIORITY 4 // Priority for light control task

// #define DEBUG

bool setup_received = false; // Flag to check if setup data has been received

/** 
 * @brief Parses the JSON string to get timeNow, days, schedule, LEDs, and values.
 * 
 * This function extracts the schedule and intensity data from the JSON string and stores them in global variables.
 * 
 * @param jsonstr The JSON string to parse.
 * @return 0 on success, -1 on failure.
 */ 
int parse_led_data(char* jsonstr) {
    // example JSON string: {"redLightStart":"12:12","redLightEnd":"13:12","redLightIntensity":10,"sunlightStart":"12:12","sunlightEnd":"18:12","sunLightIntensity":96,"Monday":true,"Tuesday":true,"Wednesday":true,"Thursday":true,"Friday":true,"Saturday":false,"Sunday":false,"timeNow":1744893150,"daylightSavingsTime":true,"timeZoneOffsetHrs":1}
    cJSON *root = cJSON_Parse(jsonstr);

    // Excessive error checking
    if (root == NULL) {
        ESP_LOGE(TAG, "Error parsing JSON string.");
        return -1;
    }
    if (!cJSON_IsObject(root)) {
        ESP_LOGE(TAG, "Error parsing JSON string. Not an object.");
        cJSON_Delete(root);
        return -1;
    }
    if (cJSON_GetObjectItem(root, "timeNow")->type != cJSON_Number) {
        ESP_LOGE(TAG, "Error parsing JSON string. Wrong time type.");
        cJSON_Delete(root);
        return -1;
    }
    if (cJSON_GetObjectItem(root, "sunLightIntensity")->type != cJSON_Number || cJSON_GetObjectItem(root, "redLightIntensity")->type != cJSON_Number || cJSON_GetObjectItem(root, "redLightStart")->type != cJSON_String || cJSON_GetObjectItem(root, "redLightEnd")->type != cJSON_String || cJSON_GetObjectItem(root, "sunlightStart")->type != cJSON_String || cJSON_GetObjectItem(root, "sunlightEnd")->type != cJSON_String) {
        ESP_LOGE(TAG, "Error parsing JSON string. Wrong schedule types.");
        cJSON_Delete(root);
        return -1;
    }
    if (strlen(cJSON_GetObjectItem(root, "redLightStart")->valuestring) != 5 || strlen(cJSON_GetObjectItem(root, "redLightEnd")->valuestring) != 5 || strlen(cJSON_GetObjectItem(root, "sunlightStart")->valuestring) != 5 || strlen(cJSON_GetObjectItem(root, "sunlightEnd")->valuestring) != 5) {
        ESP_LOGE(TAG, "Error parsing JSON string. Wrong time format.");
        cJSON_Delete(root);
        return -1;
    }
    if (cJSON_GetObjectItem(root, "sunLightIntensity")->valueint < 0 || cJSON_GetObjectItem(root, "redLightIntensity")->valueint < 0) {
        ESP_LOGE(TAG, "Error parsing JSON string. Negative intensity.");
        cJSON_Delete(root);
        return -1;
    }
    if (cJSON_GetObjectItem(root, "sunLightIntensity")->valueint > 100 || cJSON_GetObjectItem(root, "redLightIntensity")->valueint > 100) {
        ESP_LOGE(TAG, "Error parsing JSON string. Intensity over 100.");
        cJSON_Delete(root);
        return -1;
    }
    if (strcmp(cJSON_GetObjectItem(root, "redLightStart")->valuestring, cJSON_GetObjectItem(root, "redLightEnd")->valuestring) == 0 || strcmp(cJSON_GetObjectItem(root, "sunlightStart")->valuestring, cJSON_GetObjectItem(root, "sunlightEnd")->valuestring) == 0) {
        ESP_LOGE(TAG, "Error parsing JSON string. Start and end times are the same.");
        cJSON_Delete(root);
        return -1;
    }
    if (!cJSON_HasObjectItem(root, "Monday") || !cJSON_HasObjectItem(root, "Tuesday") || !cJSON_HasObjectItem(root, "Wednesday") || !cJSON_HasObjectItem(root, "Thursday") || !cJSON_HasObjectItem(root, "Friday") || !cJSON_HasObjectItem(root, "Saturday") || !cJSON_HasObjectItem(root, "Sunday")) {
        ESP_LOGE(TAG, "Error parsing JSON string. Missing day keys.");
        cJSON_Delete(root);
        return -1;
    }

    /* Store values in struct */
    
    // get timeNow and convert to string
    ledData.timeNow = cJSON_GetObjectItem(root, "timeNow")->valueint;
    convert_timeNow_to_string(ledData.timeNow, ledData.timeNowString);
    
    // get the timezone and daylight savings time
    ledData.daylightSavingsTime = cJSON_GetObjectItem(root, "daylightSavingsTime")->valueint;
    ledData.timezone = cJSON_GetObjectItem(root, "timeZoneOffsetHrs")->valueint;
    
    // copy the values intensities
    ledData.sunLightIntensity = cJSON_GetObjectItem(root, "sunLightIntensity")->valueint;
    ledData.redLightIntensity = cJSON_GetObjectItem(root, "redLightIntensity")->valueint;
    
    // copy the strings for the start and end times
    strcpy(ledData.redLightStart, cJSON_GetObjectItem(root, "redLightStart")->valuestring);
    strcpy(ledData.redLightEnd, cJSON_GetObjectItem(root, "redLightEnd")->valuestring);
    strcpy(ledData.sunlightStart, cJSON_GetObjectItem(root, "sunlightStart")->valuestring);
    strcpy(ledData.sunlightEnd, cJSON_GetObjectItem(root, "sunlightEnd")->valuestring);
    
    // set the boolean values for the days of the week
    ledData.monday = cJSON_GetObjectItem(root, "Monday")->valueint;
    ledData.tuesday = cJSON_GetObjectItem(root, "Tuesday")->valueint;
    ledData.wednesday = cJSON_GetObjectItem(root, "Wednesday")->valueint;
    ledData.thursday = cJSON_GetObjectItem(root, "Thursday")->valueint;
    ledData.friday = cJSON_GetObjectItem(root, "Friday")->valueint;
    ledData.saturday = cJSON_GetObjectItem(root, "Saturday")->valueint;
    ledData.sunday = cJSON_GetObjectItem(root, "Sunday")->valueint;

    cJSON_Delete(root);
    
    setup_received = true; // Set the flag to indicate that setup data has been received

    return 0;    
}

/**
 * @brief Prints the global variables for LED data.
 * 
 * This function logs the current LED data, including intensities and schedules, for debugging purposes.
 */
void print_led_data() {
    ESP_LOGI(TAG, "Time now: %lu", ledData.timeNow);
    ESP_LOGI(TAG, "Time now string: %s", ledData.timeNowString);
    ESP_LOGI(TAG, "Red light intensity: %d", ledData.redLightIntensity);
    ESP_LOGI(TAG, "Red light start: %s", ledData.redLightStart);
    ESP_LOGI(TAG, "Red light end: %s", ledData.redLightEnd);
    ESP_LOGI(TAG, "Sun light intensity: %d", ledData.sunLightIntensity);
    ESP_LOGI(TAG, "Sun light start: %s", ledData.sunlightStart);
    ESP_LOGI(TAG, "Sun light end: %s", ledData.sunlightEnd);
}

/**
 * @brief Initializes the global variables for LED data.
 * 
 * This function sets default values for the LED data, including intensities and schedules.
 */
void init_led_data() {
    // Timestamp
    ledData.timeNow = 0;
    ledData.timeNowString[0] = '\0';
    
    // Timezone
    ledData.daylightSavingsTime = false;
    ledData.timezone = 0; // Initialize to UTC+0
    
    // Red light
    ledData.redLightIntensity = 0;
    ledData.redLightStart[0] = '\0';
    ledData.redLightEnd[0] = '\0';

    // Sunlight
    ledData.sunLightIntensity = 0;
    ledData.sunlightStart[0] = '\0';
    ledData.sunlightEnd[0] = '\0';

    ledData.monday = false;
    ledData.tuesday = false;
    ledData.wednesday = false;
    ledData.thursday = false;
    ledData.friday = false;
    ledData.saturday = false;
    ledData.sunday = false;
}

/**
 * @brief Handles the light protocol by parsing the JSON string and storing the values in a global struct.
 * 
 * This function processes incoming JSON data to update the LED schedule and intensity settings.
 * 
 * @param write_data The JSON string containing the LED data.
 */
void handle_light_protocol(QueueData_t *data_received) {
    char jsonstr[512];
    memcpy(jsonstr, data_received->data, 512);
    jsonstr[data_received->length] = '\0'; // Fix buffer overflow issue
    ESP_LOGI(TAG, "Received JSON string: %s", jsonstr);
    if (parse_led_data(jsonstr) == 0) {
        print_led_data();
    }
}

/**
 * @brief Task to control the lights.
 * 
 * This task periodically updates the light intensities and processes incoming data.
 * 
 * @param pvParameters Parameters for the task (not used).
 */
void handle_data_task(void *pvParameters) {
    init_led_data(); // Initialize LED data
    QueueData_t received_data;

    // Start separate task for dimmer data here if setup is received 
    xTaskCreate(light_control_task, "light_control_task", LIGHT_CONTROL_STACK_SIZE, NULL, LIGHT_CONTROL_TASK_PRIORITY, NULL); // Create a new task for light control

    while (1) {
        // Wait for data from the queue
        if (xQueueReceive(shared_queue, &received_data, pdMS_TO_TICKS(1000)) == pdPASS) {
            ESP_LOGI(TAG, "Received data from queue, length: %d", received_data.length);

            // Process the received data (e.g., parse JSON, control lights)
            // Example: Log the received data
            ESP_LOG_BUFFER_HEX(TAG, received_data.data, received_data.length);
            handle_light_protocol(&received_data);
            // Send dimmer data according to the schedule
            
            // Free the allocated memory for the data
            free(received_data.data);
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
    vTaskDelete(NULL); // Delete the task when done
}

