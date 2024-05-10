#include <stdio.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <icm42670.h>

#include "icm.h"

static const char *TAG = "icm42670";
static void  icm_enable_ln_mode(void);
icm42670_t static dev = { 0 };

TaskHandle_t icm_task_handle;
QueueHandle_t icm_data_queue;
QueueHandle_t icm_action_queue;

// enable accelerometer and gyro in low-noise (LN) mode
static void  icm_enable_ln_mode(void) {
    ESP_ERROR_CHECK(icm42670_set_gyro_pwr_mode(&dev, ICM42670_GYRO_ENABLE_LN_MODE));
    ESP_ERROR_CHECK(icm42670_set_accel_pwr_mode(&dev, ICM42670_ACCEL_ENABLE_LN_MODE));
}

static void  icm_disable_power(void) {
    ESP_ERROR_CHECK(icm42670_set_gyro_pwr_mode(&dev, ICM42670_GYRO_DISABLE));
    ESP_ERROR_CHECK(icm42670_set_accel_pwr_mode(&dev, ICM42670_ACCEL_DISABLE));
}

static void  icm_process_enable_action(void) {
    icm_enable_ln_mode();
    vTaskResume(icm_task_handle);
}

static void  icm_process_disable_action(void) {
    vTaskSuspend(icm_task_handle);
    icm_disable_power();
}


static void  handle_data_task(void *pvParameters)
{
    // enable low-pass-filters on accelerometer and gyro
    ESP_ERROR_CHECK(icm42670_set_accel_lpf(&dev, ICM42670_ACCEL_LFP_53HZ));
    ESP_ERROR_CHECK(icm42670_set_gyro_lpf(&dev, ICM42670_GYRO_LFP_53HZ));
    // set output data rate (ODR)
    ESP_ERROR_CHECK(icm42670_set_accel_odr(&dev, ICM42670_ACCEL_ODR_200HZ));
    ESP_ERROR_CHECK(icm42670_set_gyro_odr(&dev, ICM42670_GYRO_ODR_200HZ));
    // set full scale range (FSR)
    ESP_ERROR_CHECK(icm42670_set_accel_fsr(&dev, ICM42670_ACCEL_RANGE_16G));
    ESP_ERROR_CHECK(icm42670_set_gyro_fsr(&dev, ICM42670_GYRO_RANGE_2000DPS));

    // now poll selected accelerometer or gyro raw value directly from registers
    while (1)
    {
        icm_data_t data_array;
        const uint8_t data_registers[6] = {
            ICM42670_REG_ACCEL_DATA_X1,
            ICM42670_REG_ACCEL_DATA_Y1,
            ICM42670_REG_ACCEL_DATA_Z1,
            ICM42670_REG_GYRO_DATA_X1,
            ICM42670_REG_GYRO_DATA_Y1,
            ICM42670_REG_GYRO_DATA_Z1
        };

        for (uint8_t i = 0; i < 6; i++) {
            int16_t raw_reading;
            ESP_ERROR_CHECK(icm42670_read_raw_data(&dev, data_registers[i], &raw_reading));
            data_array[i] = raw_reading;
            ESP_LOGI(TAG, "Raw accelerometer / gyro reading: %d", raw_reading);
        }

        xQueueSendToBack(icm_data_queue, data_array, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void  handle_action_task(void) {
    icm_action_t action;

    while (1) {
        if (xQueueReceive(icm_action_queue, &action, portMAX_DELAY)) {
            switch (action.type)
            {
            case ICM_ACTION_ENABLE:
                icm_process_enable_action();
                break;

            case ICM_ACTION_DISABLE:
                icm_process_disable_action();
                break;
            
            default:
                break;
            }
        }
    }
}


void icm_enable() {
    icm_action_t action = {
        .type = ICM_ACTION_ENABLE
    };

    xQueueSendToFront(icm_action_queue, &action, 100);
}

void icm_disable() {
    icm_action_t action = {
        .type = ICM_ACTION_DISABLE
    };

    xQueueSendToFront(icm_action_queue, &action, 100);
}

void init_icm()
{
    ESP_ERROR_CHECK(i2cdev_init());
    ESP_ERROR_CHECK(icm42670_init_desc(&dev, I2C_ADDR, PORT, 18, 19));
    ESP_ERROR_CHECK(icm42670_init(&dev));

    // FreeRtos logic
    icm_data_queue = xQueueCreate(50, sizeof(icm_data_t));
    icm_action_queue = xQueueCreate(4, sizeof(icm_action_t));
    xTaskCreate(handle_action_task, "handle_action_task", configMINIMAL_STACK_SIZE, NULL, 7, NULL);
    xTaskCreatePinnedToCore(handle_data_task, "handle_data_task", configMINIMAL_STACK_SIZE * 8, NULL, 6, &icm_task_handle, APP_CPU_NUM);
    // For default task and icm are disabled
    icm_disable();
}