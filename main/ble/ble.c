#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// BLE
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_pvcy.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "host/ble_gap.h"

#include "ble.h"
#include "ble_services.h"
#include "ble/services/hit_service.h"
#include "helpers/utils.h"

uint16_t conn_handle;
static uint8_t blehr_addr_type;
static bool is_new_device_accepted = true;
static char device_name[20] = DEFAULT_DEVICE_NAME;

static void set_ble_config(void);
static void set_default_device_name(void);
static void ble_store_config_init(void);
static void advertise(void);

static int blehr_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed */
        MODLOG_DFLT(INFO, "connection %s; status=%d\n",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);

        uint16_t rc = ble_att_set_preferred_mtu(PREFERRED_MTU_VALUE);

        if (rc != 0) {
            ESP_LOGE(BLE_TAG, "Failed to set preferred MTU; rc = %d", rc);
        }

        
        if (event->connect.status == 0) {
            conn_handle = event->connect.conn_handle;
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            print_addr(desc.peer_id_addr.val);
            is_new_device_accepted = false;
        }


        if (is_new_device_accepted == false && event->connect.status == 0) {
            if (desc.peer_id_addr.val[0] != 0xac) {
                ESP_LOGE(BLE_TAG, "Device is not accepted");
                ble_gap_terminate(conn_handle, 0x05);
                ble_store_util_delete_peer(&desc.peer_id_addr);
            }
        }

        if (event->connect.status != 0) {
            /* Connection failed; resume advertising */
            advertise();
        }

        break;


    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "disconnect; reason=%d\n", event->disconnect.reason);

        /* Connection terminated; resume advertising */
        advertise();
        break;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        MODLOG_DFLT(INFO, "repeat pairing\n");
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        return BLE_GAP_REPEAT_PAIRING_RETRY;
        break;
    
    case BLE_GAP_EVENT_AUTHORIZE:
        MODLOG_DFLT(INFO, "autorize\n");
        rc = ble_gap_conn_find(event->authorize.conn_handle, &desc);
        assert(rc == 0);
        print_addr(desc.peer_id_addr.val);

        break;
    case BLE_GAP_EVENT_IDENTITY_RESOLVED:
        MODLOG_DFLT(INFO, "autorize\n");
        rc = ble_gap_conn_find(event->identity_resolved.conn_handle, &desc);
        assert(rc == 0);
        print_addr(desc.peer_id_addr.val);

        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "adv complete\n");
        advertise();
        break;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        MODLOG_DFLT(INFO, "encryption change event; status=%d ",
                    event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        print_addr(desc.peer_id_addr.val);
        
        break;

    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        MODLOG_DFLT(INFO, "connection updated; status=%d ",
                    event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == 0);
        if (BLE_ADDR_IS_RPA(&desc.peer_id_addr) == true) {
            MODLOG_DFLT(INFO, "ADDRESS IS RPA");
        };
        print_addr(desc.peer_id_addr.val);
        MODLOG_DFLT(INFO, "\n");
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        // MODLOG_DFLT(INFO, "subscribe event; cur_notify=%d\n value handle; "
        //             "val_handle=%d\n",
        //             event->subscribe.cur_notify, svc_hits_data_handle);
        // if (event->subscribe.attr_handle == svc_hits_data_handle) {
        //     notify_state = event->subscribe.cur_notify;
        // } else if (event->subscribe.attr_handle != svc_hits_data_handle) {
        //     notify_state = event->subscribe.cur_notify;
        // }
        ESP_LOGI("BLE_GAP_SUBSCRIBE_EVENT", "conn_handle from subscribe=%d", conn_handle);
        break;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.value);
        break;
    }

    return 0;
}

/*
 * Enables advertising with parameters:
 *     o General discoverable mode
 *     o Undirected connectable mode
 */
static void advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    /*
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info)
     *     o Advertising tx power
     *     o Device name
     */
    memset(&fields, 0, sizeof(fields));

    /*
     * Advertise two flags:
     *      o Discoverability in forthcoming advertisement (general)
     *      o BLE-only (BR/EDR unsupported)
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /*
     * Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *) device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    fields.uuids16 = (ble_uuid16_t[]) {
        BLE_UUID16_INIT(HIT_SERVICE_UUID)
    };

    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;


    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(blehr_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, blehr_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

static void blehr_on_sync(void)
{
    ble_addr_t addr;
    int rc;

    /* generate new non-resolvable private address */
    rc = ble_hs_id_gen_rnd(0, &addr);
    assert(rc == 0);

    /* set generated address */
    rc = ble_hs_id_set_rnd(addr.val);
    assert(rc == 0);

    ble_hs_pvcy_rpa_config(true);
    rc = ble_hs_util_ensure_addr(1);
    assert(rc == 0);

    uint8_t own_addr_type = BLE_ADDR_RANDOM;

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(1, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    /* Begin advertising */
    advertise();
}

static void blehr_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

void blehr_host_task(void *param)
{
    ESP_LOGI(BLE_TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// TODO: Add logic for default device name storage
void set_default_device_name(void) {
    int rc = ble_svc_gap_device_name_set(DEFAULT_DEVICE_NAME);
    assert(rc == 0);

    strncpy(device_name, DEFAULT_DEVICE_NAME, sizeof(device_name));
}

void set_ble_config(void) {
    /* Initialize the NimBLE host configuration. */
    ble_hs_cfg.reset_cb = blehr_on_reset;
    ble_hs_cfg.sync_cb = blehr_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_hs_cfg.sm_io_cap = 3;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 1;
    ble_hs_cfg.sm_sc = 1;

    /* Enable the appropriate bit masks to make sure the keys
     * that are needed are exchanged
     */
    ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
}

void init_ble(void) {
    set_ble_config();
    init_ble_services();
    set_default_device_name();
    ble_store_config_init();
    nimble_port_freertos_init(blehr_host_task);
}