#define BLE_SVCS_TAG "BLE_SVCS"

int init_ble_services(void);
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);