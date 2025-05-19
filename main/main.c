#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "handle_data.h" // Custom header for data handling
#include "shared_queue.h"  // Custom shared queue header
#include "bt_component.h"  // Custom Bluetooth component header
#include "led_vars.h"      // Include the LED variables header

#define TAG "MAIN"  // Tag for logging messages

#define DATA_TASK_STACK_SIZE 4096
#define DATA_TASK_PRIORITY 5

led_data_t ledData; // Global LED data structure

QueueHandle_t shared_queue; // Define the queue handle

/**
 * @brief Main application entry point.
 *
 * This function initializes the necessary Bluetooth components and starts
 * the BLE GATT server.
 */
void app_main(void) {
    // Create the queue
    shared_queue = xQueueCreate(10, sizeof(QueueData_t));
    if (shared_queue == NULL) {
        ESP_LOGE("QUEUE", "Failed to create queue");
        return;
    }

    esp_err_t ret;

    // Initialize Non-Volatile Storage (NVS) for storing BLE configuration and state
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());  // Erase and reinitialize if NVS is full or outdated
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);  // Check for initialization errors

    // Release memory reserved for Classic Bluetooth (only BLE is used)
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // Configure the Bluetooth controller with default settings
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    // Enable Bluetooth controller in BLE mode
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    // Initialize Bluedroid stack (ESP32 Bluetooth protocol stack)
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    // Enable Bluedroid stack
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    // Register the GATT server event handler
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return;
    }

    // Register the GAP (Generic Access Profile) event handler
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return;
    }

    // Register the GATT application with a specific Application ID
    ret = esp_ble_gatts_app_register(APP_ID);
    if (ret) {
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    // Set the local MTU (Maximum Transmission Unit) for BLE communication
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(512);
    if (local_mtu_ret) {
        ESP_LOGE(TAG, "set local MTU failed, error code = %x", local_mtu_ret);
    }

    // Create the light control task
    BaseType_t task_created = xTaskCreate(handle_data_task, "Light Control Task", DATA_TASK_STACK_SIZE, NULL, DATA_TASK_PRIORITY, NULL);
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Light Control Task");
        return;
    } else {
        ESP_LOGI(TAG, "Light Control Task created successfully");
    }

    return;  // End of main function
}
