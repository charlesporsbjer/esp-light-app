#ifndef BT_VARS_H
#define BT_VARS_H

#include "esp_mac.h"
#include "bt_component.h"

// Structure to handle prepared writes in GATT operations
prepare_type_env_t prepare_write_env;

// Characteristic properties: allows read, write, and notify operations
esp_gatt_char_prop_t property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

// Example characteristic value (3 bytes)
uint8_t char1_str[] = {0xEA, 0x7A, 0x55};

// Flag to track advertising configuration progress
uint8_t adv_config_done = 0;

// GATT characteristic value structure, initialized with `char1_str`
esp_attr_value_t gatts_demo_char1_val =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

// 128-bit UUID for the advertised service, contains two UUIDs for different purposes
uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

// BLE advertising data configuration
// The length of adv data must be less than 31 bytes
// static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// BLE scan response data configuration
esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// Advertising parameters configuration
esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// Structure representing a GATT profile instance
extern struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;        // Callback function for GATT server events
    uint16_t gatts_if;              // GATT interface ID
    uint16_t app_id;                // Application ID
    uint16_t conn_id;               // Connection ID for the current client
    uint16_t service_handle;        // Handle for the primary service
    esp_gatt_srvc_id_t service_id;  // Service ID structure

    // Characteristic handles for different functionalities
    uint16_t char_handle_tx;        // Handle for TX characteristic
    uint16_t char_handle_rx;        // Handle for RX characteristic

    // UUIDs for the characteristics
    esp_bt_uuid_t char_uuid_tx;
    esp_bt_uuid_t char_uuid_rx;

    uint16_t client_id;             // ID of the connected client
    uint16_t client_handle;         // Handle for the client
    bool remote_notify_enabled;     // Flag to indicate if notifications are enabled
    esp_gatt_perm_t perm;           // Permissions for GATT attributes
    esp_gatt_char_prop_t property;  // Properties of the characteristic
    uint16_t descr_handle;          // Handle for the characteristic descriptor
    esp_bt_uuid_t descr_uuid;       // UUID for the descriptor
};

// GATT profile instance, initially set to an invalid interface
struct gatts_profile_inst gl_profile = {
    .gatts_if = ESP_GATT_IF_NONE
};

#endif