#include <esp_err.h>
#include "nvs_flash.h"

// #include "storage/ble_storage.h"


void init_storage(void) {
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    // TOOD: delete after testing
    // ESP_ERROR_CHECK(nvs_flash_erase());
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    // ble_storage_init();
}