#include "esp_log.h"
#include "freertos/FreeRTOSConfig.h"

// BLE
#include "host/ble_hs.h"

#include "manufacture_service.h"
#include "ble/ble.h"

static const char *manuf_name = "TenisTrainer";
static const char *model_num = "V1.0";

static int access_device_info_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

const struct ble_gatt_svc_def device_info_svc = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = BLE_UUID16_DECLARE(DEVICE_INFO_SVC_UUID),
    .characteristics = (struct ble_gatt_chr_def[])
    { {
            .uuid = BLE_UUID16_DECLARE(MANUFACTURER_NAME_CHR_UUID),
            .access_cb = access_device_info_data_cb,
            .flags =  BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
        },
        {
            .uuid = BLE_UUID16_DECLARE(MODEL_NUMBER_CHR_UUID),
            .access_cb = access_device_info_data_cb,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
        }, 
        {
            0, /* No more characteristics in this service */
        },
    }
};

static int access_device_info_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    
    uint16_t uuid;
    int is_error;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == MANUFACTURER_NAME_CHR_UUID) {
        is_error = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == MODEL_NUMBER_CHR_UUID) {
        is_error = os_mbuf_append(ctxt->om, model_num, strlen(model_num));

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}