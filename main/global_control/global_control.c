#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_log.h"

#include "main.h"
#include "global_control.h"
#include "icm/icm.h"
#include "hit_recording_handler/hit_recording_handler.h"

QueueHandle_t global_actions_queue;
const static char* TAG = "[GLOBAL ACTION]";

// State flags
static bool is_device_in_deep_sleep = false;
static bool is_hit_recording_now = false;

static void process_deep_sleep_on_action(void) {
    if (is_device_in_deep_sleep) {
        return;
    }

    ESP_LOGI(TAG, "deep sleep on");
    is_device_in_deep_sleep = true;
}

static void process_deep_sleep_off_action(void) {
    if (!is_device_in_deep_sleep) {
        return;
    }

    ESP_LOGI(TAG, "deep sleep off");
    is_device_in_deep_sleep = false;
}

#if IS_DATA_COLLECTOR_DEVICE
    static void process_start_recording_hit_action(void) {
        if (is_hit_recording_now) {
            return;
        }

        icm_enable();
        is_hit_recording_now = true;
    }

    static void process_end_recording_hit_action(void) {
        if (!is_hit_recording_now) {
            return;
        }

        icm_disable();
        hit_save_recording();
        is_hit_recording_now = false;
    }
#endif

static void global_control_action_checker_task(void *pvParams) {
    global_actions_queue = xQueueCreate(10, sizeof(global_action_t));
    global_action_t action;

    while (1) {
        if (xQueueReceive(global_actions_queue, &action, portMAX_DELAY)) {
            switch (action.type)
            {
            case ACTION_DEEP_SLEEP_ON:
                process_deep_sleep_on_action();
                break;

            case ACTION_DEEP_SLEEP_OFF:
                process_deep_sleep_off_action();
                break;


            #if IS_DATA_COLLECTOR_DEVICE
                case ACTION_START_RECORDING_HIT:
                    process_start_recording_hit_action();
                    break;

                case ACTION_END_RECORDING_HIT:
                    process_end_recording_hit_action();
                    break;
            #endif

            default:
                break;
            }
        } else {
            ESP_LOGE(TAG, "Action is not received from queue");
        }
    }
}

static void send_action_to_queue(global_action_type_t type) {
    global_action_t action = {
        .type = type
    };

    xQueueSendToFront(global_actions_queue, &action, 100);
}

void deep_sleep_on(void) {
    send_action_to_queue(ACTION_DEEP_SLEEP_ON);
}

void deep_sleep_off(void) {
   send_action_to_queue(ACTION_DEEP_SLEEP_OFF);
}

void toggle_deep_sleep(void) {
    if (is_device_in_deep_sleep) {
        deep_sleep_off();
    } else {
        deep_sleep_on();
    }
}

#if IS_DATA_COLLECTOR_DEVICE
    void start_recording_hit(void) {
        send_action_to_queue(ACTION_START_RECORDING_HIT);
    }

    void end_recording_hit(void) {
        send_action_to_queue(ACTION_END_RECORDING_HIT);
    }

    void toggle_recording_hit(void) {
        if (is_hit_recording_now) {
            end_recording_hit();
        } else {
            start_recording_hit();
        }
    }
#endif

void init_global_control(void) {
    xTaskCreate(global_control_action_checker_task, "action_checker_task", configMINIMAL_STACK_SIZE, NULL, 8, NULL);
}