/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "blehr_sens.h"

#include "main.h"

static const char *manuf_name = "Apache Mynewt ESP32 devkitC";
static const char *model_num = "Mynewt HR Sensor demo";
uint16_t svc_hits_data_handle;
uint16_t svc_hits_is_sync_start_handle;

// DATA
uint8_t is_new_data_exist = 1;
uint8_t current_type = 1;
uint8_t is_sync_start = 0;

static int
gatt_svr_chr_access_hits_data(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);
                                
void start_task_hits_data_sync(void);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: Heart-rate */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_SVC_HITS_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                .uuid = BLE_UUID16_DECLARE(GATT_SVC_HITS_DATA_CHR_UUID),
                .access_cb = gatt_svr_chr_access_hits_data,
                .val_handle = &svc_hits_data_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
            },
            {
                .uuid = BLE_UUID16_DECLARE(GATT_SVC_HITS_IS_NEW_DATA_CHR_UUID),
                .access_cb = gatt_svr_chr_access_hits_data,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
            }, 
            {
                .uuid = BLE_UUID16_DECLARE(GATT_SVC_HITS_IS_SYNC_IN_ROGRESS_CHR_UUID),
                .access_cb = gatt_svr_chr_access_hits_data,
                .val_handle = &svc_hits_is_sync_start_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC | BLE_GATT_CHR_F_NOTIFY,
            },
            {
                .uuid = BLE_UUID16_DECLARE(GATT_SVC_HITS_CURRENT_TYPE_CHR_UUID),
                .access_cb = gatt_svr_chr_access_hits_data,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
            },
            {
                0, /* No more characteristics in this service */
            },
        }
    },

    {
        /* Service: Device Information */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_DEVICE_INFO_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                /* Characteristic: * Manufacturer name */
                .uuid = BLE_UUID16_DECLARE(GATT_MANUFACTURER_NAME_UUID),
                .access_cb = gatt_svr_chr_access_device_info,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
            }, {
                /* Characteristic: Model number string */
                .uuid = BLE_UUID16_DECLARE(GATT_MODEL_NUMBER_UUID),
                .access_cb = gatt_svr_chr_access_device_info,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
            }, {
                0, /* No more characteristics in this service */
            },
        }
    },

    {
        0, /* No more services */
    },
};

static int
gatt_svr_chr_access_hits_data(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    
    uint16_t uuid;
    int is_error;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == GATT_SVC_HITS_IS_NEW_DATA_CHR_UUID) {
        is_error = os_mbuf_append(ctxt->om, &is_new_data_exist, sizeof(is_new_data_exist));

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_SVC_HITS_CURRENT_TYPE_CHR_UUID && ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        is_error = os_mbuf_append(ctxt->om, &current_type, sizeof(current_type));

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_SVC_HITS_IS_SYNC_IN_ROGRESS_CHR_UUID && ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        is_error = os_mbuf_append(ctxt->om, &is_sync_start, sizeof(is_sync_start));

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_SVC_HITS_CURRENT_TYPE_CHR_UUID && ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        is_error = ble_hs_mbuf_to_flat(ctxt->om, &current_type, sizeof(current_type), NULL);

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_SVC_HITS_IS_SYNC_IN_ROGRESS_CHR_UUID && ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        is_error = ble_hs_mbuf_to_flat(ctxt->om, &is_sync_start, sizeof(is_sync_start), NULL);

        if (is_sync_start == 1) {
            start_task_hits_data_sync();
        }

        return is_error == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void task_hits_data_sync(void *pvParameters) {
    uint8_t data[] = {1, 2 ,3, 1, 2, 1, 1};
    uint16_t current_mtu = ble_att_mtu(conn_handle) - 3;

    printf("current mtu: %d\n", current_mtu);

    for (uint16_t i = 0; i < sizeof(data); i = i + current_mtu) {
        uint8_t buffer[current_mtu];
        memset(&buffer, 0, sizeof(buffer));

        for (uint16_t j = 0; j < current_mtu; j++) {
            if (i + j < sizeof(data)) {
                buffer[j] = data[i + j];
            }
        }

        struct os_mbuf *om = ble_hs_mbuf_from_flat(buffer, current_mtu);
        ble_gatts_notify_custom(conn_handle, svc_hits_data_handle, om);
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    is_sync_start = 0;
    is_new_data_exist = 0;
    ble_gatts_chr_updated(svc_hits_is_sync_start_handle);
    vTaskDelete(NULL);
}


void start_task_hits_data_sync(void) {
    xTaskCreate(task_hits_data_sync, "task_hits_data_sync", 4096, NULL, 5, NULL);
}

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == GATT_MODEL_NUMBER_UUID) {
        rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_MANUFACTURER_NAME_UUID) {
        rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(DEBUG, "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int
gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }


    return 0;
}
