#include <stdint.h>
#include "esp_err.h"

#define BLE_STORAGE_NAME                    "ble_storage"
#define BLE_STORAGE_PAIRED_DEVICES_KEY      "pd"
#define BLE_DEVICES_DATA_LENGTH             18

/** @brief Check if some saved devices exist */
bool is_some_saved_deviced_exist(void);
/** @brief Get saved device addresses */
esp_err_t get_saved_device_addresses(uint8_t *devices, uint8_t *is_empty);
/** @brief Save paired device address */
esp_err_t save_paired_device(uint8_t device[6]);