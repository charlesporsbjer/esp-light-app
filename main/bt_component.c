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
#include "sdkconfig.h"
#include "freertos/queue.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"


#include "bt_vars.h"        // Include the shared variables header
#include "handle_data.h"   // Custom header for light handling
#include "shared_queue.h"   // Include the shared queue header

#define TAG "BT_COMPONENT"


/**
 * @brief Sends a search notification to a connected BLE client.
 * 
 * @param gatts_if The GATT server interface.
 * @param client_id The ID of the connected client.
 * @param client_handle The handle of the characteristic to send the notification from.
 * @param len The length of the data to be sent (typically 1).
 * @param data Pointer to the data buffer to be sent.
 */
void send_search_notification(esp_gatt_if_t gatts_if, uint16_t client_id, uint16_t client_handle, uint8_t len, uint8_t *data) {
    esp_ble_gatts_send_indicate(gatts_if, client_id, client_handle, len, data, false);
}


/**
 * @brief Handles write events for BLE GATT server, including prepare writes and responses.
 * 
 * @param gatts_if GATT interface handle.
 * @param prepare_write_env Pointer to the environment for prepared writes.
 * @param param Pointer to event parameters.
 */
void write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(TAG, "handling prepared write event)");
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp) {
        if (param->write.is_prep) {
            if (param->write.offset > PREPARE_BUF_MAX_SIZE) {
                status = ESP_GATT_INVALID_OFFSET;
            }
            if (status == ESP_GATT_OK && prepare_write_env->prepare_buf == NULL) {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL) {
                    ESP_LOGE(TAG, "Gatt_server prep malloc failed, no memory");
                    status = ESP_GATT_NO_RESOURCES;
                }
            }
            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            if (gatt_rsp) {
                gatt_rsp->attr_value.len = param->write.len;
                gatt_rsp->attr_value.handle = param->write.handle;
                gatt_rsp->attr_value.offset = param->write.offset;
                gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
                memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
                esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
                if (response_err != ESP_OK) {
                    ESP_LOGE(TAG, "Send resonse error}\n");
                }
                free(gatt_rsp);
            } else {
                ESP_LOGE(TAG, "malloc failed, no resource to send response error\n");
                status = ESP_GATT_NO_RESOURCES;
            }
            if (status != ESP_GATT_OK) {
                return;
            }
            mempcpy(prepare_write_env->prepare_buf + param ->write.offset, param->write.value, param->write.len);
            prepare_write_env->prepare_len += param->write.len;
        } else {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
} 

/**
 * @brief Executes or cancels a prepared write event.
 * 
 * @param prepare_write_env Pointer to the environment for prepared writes.
 * @param param Pointer to event parameters.
 */
void exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t * param) {
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC) {
        ESP_LOG_BUFFER_HEX(TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    } else {
        ESP_LOGI(TAG, "ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

/**
 * @brief Handles GAP events such as advertisement start and scan response setup.
 * 
 * @param event GAP BLE event type.
 * @param param Pointer to event parameters.
 */
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT");
            adv_config_done &= (~adv_config_flag);
            if (adv_config_done == 0) {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_DATA_SET_COMPLETE_EVT");
            adv_config_done &= (~scan_rsp_config_flag);
            if (adv_config_done == 0) {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_START_COMPLETE_EVT");
            // Advertising start complete event to indicate advertising start successfully or failed.
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising failed to start, error code = %x", param->adv_start_cmpl.status);
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d, conn_int = %d, latency = %d, timeout = %d",
                    param->update_conn_params.status,
                    param->update_conn_params.min_int,
                    param->update_conn_params.max_int,
                    param->update_conn_params.conn_int,
                    param->update_conn_params.latency,
                    param->update_conn_params.timeout);
            break;
        case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
            ESP_LOGI(TAG, "packet length updated: rx = %d, tx = %d, status = %d",
                    param->pkt_data_length_cmpl.params.rx_len,
                    param->pkt_data_length_cmpl.params.tx_len,
                    param->pkt_data_length_cmpl.status);
            break;
        case ESP_GAP_BLE_PHY_UPDATE_COMPLETE_EVT: {
            // Access the PHY update parameters from the callback parameter
            if (param->phy_update.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "PHY update complete: TX PHY = %d, RX PHY = %d", param->phy_update.tx_phy, param->phy_update.rx_phy);
            } else {
                ESP_LOGE(TAG, "PHY update failed with status %d", param->phy_update.status);
            }
            break;
        }
        default:
            ESP_LOGI(TAG, "unknown gap event = %d", event);
            break;
    }
}

/**
 * @brief Handles GATT server events such as registration, read, write, and MTU updates.
 * 
 * @param event GATT server event type.
 * @param gatts_if GATT interface handle.
 * @param param Pointer to event parameters.
 */
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile.gatts_if = gatts_if;
        } else {
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
    }
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
        gl_profile.service_id.is_primary = true;
        gl_profile.service_id.id.inst_id = 0x00;
        gl_profile.service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile.service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID;

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME); 
        if (set_dev_name_ret){
            ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }

        //config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret){
            ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
        }
        //config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret){
            ESP_LOGE(TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;
        
        esp_ble_gatts_create_service(gatts_if, &gl_profile.service_id, GATTS_NUM_HANDLE);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(TAG, "GATT_READ_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);

        if (param->write.handle == gl_profile.char_handle_tx) {
            if (!param->write.is_prep) {
                // Normal write: Enqueue the data directly
                QueueData_t queue_data;
                queue_data.length = param->write.len;
                queue_data.data = malloc(queue_data.length);

                if (queue_data.data != NULL) {
                    memcpy(queue_data.data, param->write.value, queue_data.length);
                    if (xQueueSend(shared_queue, &queue_data, pdMS_TO_TICKS(100)) != pdPASS) {
                        ESP_LOGE(TAG, "Failed to enqueue data");
                        free(queue_data.data);
                    } else {
                        ESP_LOGI(TAG, "Data enqueued successfully");
                    }
                } else {
                    ESP_LOGE(TAG, "Failed to allocate memory for queue data");
                }
            } else {
                // Prepared write: Handle buffering in write_event_env
                write_event_env(gatts_if, &prepare_write_env, param);
            }
        }

        // Send a response if needed
        if (param->write.need_rsp) {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        }
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        exec_write_event_env(&prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT: 
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d, service_handle %d", param->create.status, param->create.service_handle);
        gl_profile.service_handle = param->create.service_handle;
        // gl_profile.char_uuid_rx.len = ESP_UUID_LEN_16;
        // gl_profile.char_uuid_rx.uuid.uuid16 = GATTS_CHAR_UUID_RX;

        gl_profile.char_uuid_tx.len = ESP_UUID_LEN_16;
        gl_profile.char_uuid_tx.uuid.uuid16 = GATTS_CHAR_UUID_TX;
        property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_char_tx_ret = esp_ble_gatts_add_char(gl_profile.service_handle, &gl_profile.char_uuid_tx, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, property, &gatts_demo_char1_val, NULL);
        if (add_char_tx_ret) {
            ESP_LOGE(TAG, "add char_tx failed, error code =%x", add_char_tx_ret);
        }

        gl_profile.char_uuid_rx.len = ESP_UUID_LEN_16;
        gl_profile.char_uuid_rx.uuid.uuid16 = GATTS_CHAR_UUID_RX;
        property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_char_rx_ret = esp_ble_gatts_add_char(gl_profile.service_handle, &gl_profile.char_uuid_rx, 
                    ESP_GATT_PERM_READ , property, 
                    &gatts_demo_char1_val, NULL);
        if (add_char_rx_ret) {
            ESP_LOGE(TAG, "add char_rx failed, error code =%x", add_char_rx_ret);
        }

        esp_ble_gatts_start_service(gl_profile.service_handle);
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t length = 0;
        const uint8_t *prf_char;
        ESP_LOGI(TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d", param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);        
        // kolla vilken char på uuid. spara handle till den char
        //gl_profile.descr_handle = param->add_char.attr_handle;
        gl_profile.descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile.descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
        if (get_attr_ret == ESP_FAIL) {
            ESP_LOGE(TAG, "ILLEGAL HANDLE: Unable to get attribute value");
        } 
        ESP_LOGI(TAG, "the gatts demo char length = %x", length);
        for (int i = 0; i < length; i++) {
            ESP_LOGI(TAG, "prf_char[%x] =%x",i,prf_char[i]);
        }
        if (param->add_char_descr.descr_uuid.uuid.uuid16 == GATTS_CHAR_UUID_TX) {
            gl_profile.char_handle_tx = param->add_char.attr_handle;
            ESP_LOGI(TAG, "TX characteristic added, handle: %d", gl_profile.char_handle_tx);
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(
                gl_profile.service_handle,                   // Service handle
                &gl_profile.descr_uuid,                           // Descriptor UUID (CCCD)
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,         // Permissions
                NULL,                                             // Value (can be NULL)
                NULL                                              // Control block
            );
            if (add_descr_ret) {
                ESP_LOGE(TAG, "add char descr failed, error code =%x", add_descr_ret);
            } else {
                ESP_LOGI(TAG, "DESCR characteristic added");
            }

        } else if (param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_RX) {
            gl_profile.char_handle_rx = param->add_char.attr_handle;
            ESP_LOGI(TAG, "RX characteristic added, handle: %d", gl_profile.char_handle_rx);
        } else {
            ESP_LOGW(TAG, "Uknown characteristic UUID: 0x%04x", param->add_char.char_uuid.uuid.uuid16);
        }

        ESP_LOG_BUFFER_HEX(TAG, prf_char, length);
        
        break;
        }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gl_profile.descr_handle = param->add_char_descr.attr_handle;    
        ESP_LOGI(TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d", param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_CONNECT_EVT: {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20;     // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;     // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;      // timeout = 400*10ms = 4000ms
        ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x, handle %d",
                param->connect.conn_id,
                param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5], param->connect.conn_handle);
        gl_profile.conn_id = param->connect.conn_id;
        

        // Make sure to only save client's info for writing 
        gl_profile.client_handle = param->connect.conn_handle;
        gl_profile.client_id = param->connect.conn_id;
        ESP_LOGI(TAG, "client_handle %d, client_id %d saved to gl_profile", gl_profile.client_handle, gl_profile.client_id);

        
        // Start sent the update connection to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        
        esp_ble_gap_start_advertising(&adv_params); // START ADVERTISING AGAIN
        break;
        }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            ESP_LOG_BUFFER_HEX(TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_RESPONSE_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_RESPONSE_EVT: Response sent successfully");
        break;
    default:
        ESP_LOGI(TAG, "unknown gatts event = %d", event);
        break;
    }
}