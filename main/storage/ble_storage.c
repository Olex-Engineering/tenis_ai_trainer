#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nvs_flash.h"
#include "esp_err.h"

#include "ble_storage.h"

bool is_some_saved_deviced_exist(void) {
    nvs_handle_t ble_storage_handle;
    esp_err_t err;
    uint8_t data_length;

    err = nvs_open(BLE_STORAGE_NAME, NVS_READONLY, &ble_storage_handle);
    if (err != ESP_OK) return false;

    err = nvs_get_blob(ble_storage_handle, BLE_STORAGE_PAIRED_DEVICES_KEY, NULL, &data_length);
    if (err != ESP_OK) return false;

    return true;
    nvs_close(ble_storage_handle);
}

esp_err_t get_saved_device_addresses(uint8_t *devices, uint8_t *devices_length) {
    esp_err_t err;
    nvs_handle_t ble_storage_handle;
    size_t data_length = 0;
    uint8_t data_length_count = 0;

    err = nvs_open(BLE_STORAGE_NAME, NVS_READONLY, &ble_storage_handle);
    if (err != ESP_OK) return err;
    err = nvs_get_blob(ble_storage_handle, BLE_STORAGE_PAIRED_DEVICES_KEY, NULL, &data_length);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    printf("data length %d \n", data_length);
    if (data_length == 0) {
        *devices_length = 0;
        return ESP_OK;
    }

    uint8_t *saved_devices = (uint8_t *) calloc(data_length, 1);
    
    err = nvs_get_blob(ble_storage_handle, BLE_STORAGE_PAIRED_DEVICES_KEY, saved_devices, &data_length);
    if (err != ESP_OK) return err;

    for (uint8_t i = 0; i < data_length; i += 6) {
        if (saved_devices[i] == 0 && saved_devices[i + 1] == 0) {
            continue;
        }

        data_length_count++;
    }

    *devices_length = data_length_count;
    memcpy(devices, saved_devices, data_length);
    free(saved_devices);

    err = nvs_commit(ble_storage_handle);
    if (err != ESP_OK) return err;
    nvs_close(ble_storage_handle);
    return ESP_OK;
}

esp_err_t save_paired_device(uint8_t device[6]) {
    esp_err_t err;
    nvs_handle_t ble_storage_handle;
    uint8_t saved_devices[BLE_DEVICES_DATA_LENGTH] = {0};
    size_t saved_devices_count = 0;

    err = get_saved_device_addresses(saved_devices, &saved_devices_count);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;


    err = nvs_open(BLE_STORAGE_NAME, NVS_READWRITE, &ble_storage_handle);
    if (err != ESP_OK) return err;

    if (saved_devices_count == 3) {
        for (uint8_t i = 6; i < BLE_DEVICES_DATA_LENGTH; i++) {
            saved_devices[i - 6] = saved_devices[i];
        }

        for (uint8_t i = 12; i < BLE_DEVICES_DATA_LENGTH; i++) {
            saved_devices[i] = device[i - 12];
        }

        saved_devices_count = 2;
    }

    for (uint8_t i = 0; i < 6; i++) {
        saved_devices[i + saved_devices_count * 6] = device[i];
    }

    printf("Saved devices: %d\n", saved_devices[0]);    
    err = nvs_set_blob(ble_storage_handle, BLE_STORAGE_PAIRED_DEVICES_KEY, &saved_devices, BLE_DEVICES_DATA_LENGTH);
    if (err != ESP_OK) return err;
    err = nvs_commit(ble_storage_handle);
    if (err != ESP_OK) return err;
    nvs_close(ble_storage_handle);
    printf("Saved ok");    
    return ESP_OK;
}