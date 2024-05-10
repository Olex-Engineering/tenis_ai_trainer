#include <stdbool.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "icm/icm.h"
#include "esp_log.h"
#include "hit_recording_handler/hit_recording_handler.h"

static const char* TAG = "[HIT RECORDING]";

static int16_t* single_hit_data = NULL;
static uint32_t single_hit_data_length = 0;

static QueueHandle_t action_queue = NULL;

static void  process_abort_recording_action(void) {
    single_hit_data_length = 0;
}

static void  process_save_single_hit_data(void) {
    // TODO: add logic to save into the storage

    single_hit_data_length = 0;
}


static void  handle_data_from_icm(icm_data_t* data) {
    if (single_hit_data_length + ICM_DATA_LENGTH > MAX_SINGLE_HIT_DATA_LENGTH) {
        ESP_LOGE(TAG, "The size of single hit are too big. Aborting recording.");
        hit_abort_recording();
        return;
    }

    memcpy(&single_hit_data[single_hit_data_length], data, ICM_DATA_LENGTH);
    single_hit_data_length += ICM_DATA_LENGTH;
}

static void  handle_icm_data_task(void *pvParams) {
    icm_data_t icm_data;
    single_hit_data = (int16_t*) malloc(MAX_SINGLE_HIT_DATA_LENGTH);

    while (1) {
        if (xQueueReceive(icm_data_queue, &icm_data, portMAX_DELAY)) {
            handle_data_from_icm(&icm_data);
        } else {
            ESP_LOGE(TAG, "Item is not received from data queue");
        }
    }
}

static void  handle_action_task(void *pvParams) {
    hit_recording_action_t action;
    
    while (1) {
        if (xQueueReceive(action_queue, &action, portMAX_DELAY)) {
            switch (action.type)
            {
            case HIT_RECORDING_ACTION_SAVE_HIT:
                process_save_single_hit_data();
                break;

            case HIT_RECORDING_ACTION_ABORT_HIT:
                process_abort_recording_action();
                break;
            
            default:
                break;
            }
        } else {
            ESP_LOGE(TAG, "Item is not received from action queue");
        }
    }
}

void hit_abort_recording() {
    hit_recording_action_t action = {
        .type = HIT_RECORDING_ACTION_ABORT_HIT
    };

    xQueueSendToFront(action_queue, &action, 10);
}

void hit_save_recording() {
    hit_recording_action_t action = {
        .type = HIT_RECORDING_ACTION_SAVE_HIT
    };

    xQueueSendToFront(action_queue, &action, 10);
}

void init_hit_recording(void) {
    action_queue = xQueueCreate(8, sizeof(hit_recording_action_t));
    xTaskCreate(handle_icm_data_task, "handle_icm_data", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);
    xTaskCreate(handle_action_task, "handl_action", configMINIMAL_STACK_SIZE * 2, NULL, 8, NULL);
}