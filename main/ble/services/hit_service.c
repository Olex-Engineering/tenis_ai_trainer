#include "esp_log.h"
#include "freertos/FreeRTOSConfig.h"

// BLE
#include "host/ble_hs.h"

#include "hit_service.h"
#include "ble/ble.h"

static void start_task_data_sync(void);
static int access_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

static uint16_t data_handle;
static uint16_t is_sync_start_handle;

// FLAGS

// Will be set after data is checked init function
uint8_t is_new_data_exist =     HIT_IS_NEW_DATA_EXIST_DEFAULT;

uint8_t current_type =          HIT_CURRENT_TYPE_DEFAULT;
uint8_t is_sync_start =         HIT_IS_SYNC_IN_PROGRESS_DEFAULT;

const struct ble_gatt_svc_def hit_service = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = BLE_UUID16_DECLARE(HIT_SERVICE_UUID),
    .characteristics = (struct ble_gatt_chr_def[])
    { {
            .uuid = BLE_UUID16_DECLARE(HIT_SERVICE_DATA_CHR_UUID),
            .access_cb = access_data_cb,
            .val_handle = &data_handle,
            .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
        },
        {
            .uuid = BLE_UUID16_DECLARE(HIT_SERVICE_IS_NEW_DATA_CHR_UUID),
            .access_cb = access_data_cb,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
        }, 
        {
            .uuid = BLE_UUID16_DECLARE(HIT_SERVICE_IS_SYNC_IN_ROGRESS_CHR_UUID),
            .access_cb = access_data_cb,
            .val_handle = &is_sync_start_handle,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC | BLE_GATT_CHR_F_NOTIFY,
        },
        {
            .uuid = BLE_UUID16_DECLARE(HIT_SERVICE_CURRENT_TYP_CHR_UUID),
            .access_cb = access_data_cb,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
        },
        {
            0, /* No more characteristics in this service */
        },
    }
};

static int access_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    
    uint16_t uuid;
    int is_error;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == HIT_SERVICE_IS_NEW_DATA_CHR_UUID) {
        is_error = os_mbuf_append(ctxt->om, &is_new_data_exist, sizeof(is_new_data_exist));

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == HIT_SERVICE_CURRENT_TYP_CHR_UUID && ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        is_error = os_mbuf_append(ctxt->om, &current_type, sizeof(current_type));

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == HIT_SERVICE_IS_SYNC_IN_ROGRESS_CHR_UUID && ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        is_error = os_mbuf_append(ctxt->om, &is_sync_start, sizeof(is_sync_start));

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == HIT_SERVICE_CURRENT_TYP_CHR_UUID && ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        is_error = ble_hs_mbuf_to_flat(ctxt->om, &current_type, sizeof(current_type), NULL);

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == HIT_SERVICE_IS_SYNC_IN_ROGRESS_CHR_UUID && ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        is_error = ble_hs_mbuf_to_flat(ctxt->om, &is_sync_start, sizeof(is_sync_start), NULL);

        if (is_sync_start == 1) {
            start_task_data_sync();
        } else {
            return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
        }

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void data_sync(void *pvParameters) {
    // TODO: Test data is used
    uint8_t data[] = {1, 2 ,3, 1, 2, 1, 1};
    uint16_t current_mtu = ble_att_mtu(conn_handle) - 3;

    ESP_LOGE(HIT_SERVICE_TAG, "current mtu: %d\n", current_mtu);

    for (uint16_t i = 0; i < sizeof(data); i = i + current_mtu) {
        uint8_t buffer[current_mtu];
        memset(&buffer, 0, sizeof(buffer));

        for (uint16_t j = 0; j < current_mtu; j++) {
            if (i + j < sizeof(data)) {
                buffer[j] = data[i + j];
            }
        }

        struct os_mbuf *om = ble_hs_mbuf_from_flat(buffer, current_mtu);
        ble_gatts_notify_custom(conn_handle, data_handle, om);
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    is_sync_start = 0;
    is_new_data_exist = 0;
    ble_gatts_chr_updated(is_sync_start_handle);
    vTaskDelete(NULL);
}


void start_task_data_sync(void) {
    xTaskCreate(data_sync, "data_sync", 4096, NULL, 5, NULL);
}