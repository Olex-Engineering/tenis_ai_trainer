#include <assert.h>

#include "esp_log.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "host/ble_gatt.h"

// Services
#include "ble/ble.h"
#include "ble/ble_services.h"
#include "ble/services/hit_service.c"

const struct ble_gatt_svc_def ble_services[] = {
    hit_service,
    {
        0,// End of services
    }
};

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGE(BLE_SVCS_TAG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGE(BLE_SVCS_TAG, "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGE(BLE_SVCS_TAG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int init_ble_services(void) {
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(ble_services);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(ble_services);
    if (rc != 0) {
        return rc;
    }


    return 0;
}