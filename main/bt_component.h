#ifndef BT_COMPONENT_H
#define BT_COMPONENT_H

#include "stdint.h"
#include "stdbool.h"
#include "esp_bt_defs.h"
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"

// Unique Application ID for the BLE service
#define APP_ID                      0

// Number of handles allocated for the GATT service
#define GATTS_NUM_HANDLE            15

// UUIDs for the GATT service and characteristics
#define GATTS_SERVICE_UUID          0xB00B  // Primary service UUID
#define GATTS_CHAR_UUID_TX          0xFEED  // TX characteristic (ESP to client)
#define GATTS_CHAR_UUID_RX          0xEA7A  // RX characteristic (Client to ESP)
#define GATTS_DESCR_UUID            0x2902  // Descriptor UUID for notifications

// Device name advertised over BLE
#define DEVICE_NAME                 "esp_nordic_light"

// Maximum length of a characteristic value
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40  // 64 bytes

// Maximum buffer size for prepared write operations
#define PREPARE_BUF_MAX_SIZE        1024

// Flags to track advertising and scan response configuration
#define adv_config_flag             (1 << 0) // Bit flag for advertising configuration
#define scan_rsp_config_flag        (1 << 1) // Bit flag for scan response configuration


// Structure for handling prepared write operations
typedef struct {
    uint8_t  *prepare_buf;          // Pointer to the buffer for prepared data
    int      prepare_len;           // Length of the prepared data
} prepare_type_env_t;

/**
 * @brief Sends a notification or indication to the connected client.
 *
 * @param gatts_if       GATT interface ID.
 * @param remote_id      Connection ID of the remote client.
 * @param remote_handle  Handle of the characteristic to notify.
 * @param len            Length of the data.
 * @param data           Pointer to the data to send.
 */
void send_search_notification(esp_gatt_if_t gatts_if, uint16_t remote_id, uint16_t remote_handle, uint8_t len, uint8_t *data);

/**
 * @brief Handles write events by storing data in a prepared write buffer.
 *
 * @param gatts_if          GATT interface ID.
 * @param prepare_write_env Pointer to the prepared write environment structure.
 * @param param             GATT write event parameters.
 */
void write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

/**
 * @brief Executes a write operation after a prepared write request.
 *
 * @param prepare_write_env Pointer to the prepared write environment structure.
 * @param param             GATT execute write event parameters.
 */
void exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

/**
 * @brief Handles GAP (Generic Access Profile) events.
 *
 * @param event GAP event type.
 * @param param GAP event parameters.
 */
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

/**
 * @brief Handles GATT server events.
 *
 * @param event   GATT event type.
 * @param gatts_if GATT interface ID.
 * @param param   GATT event parameters.
 */
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#endif // BT_COMPONENT_H
