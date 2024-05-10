#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/touch_pad.h"

#include "global_control/global_control.h"
#include "touch_control.h"

static const char *TAG = "[TOUCH PAD]";

static QueueHandle_t que_touch = NULL;

// Timer
static bool is_long_press_timer_in_progress = false;
TimerHandle_t long_press_timer_handle;

static void long_press_timer_cb(void *arg) {
    is_long_press_timer_in_progress = false;
}

static void start_long_press_timer(void) {
    is_long_press_timer_in_progress = true;

    // Start timer for 3 seconds
    ESP_ERROR_CHECK(esp_timer_start_once(long_press_timer_handle, LONG_PRESS_MICRO_SECONDS_AMOUNT));
}

static void process_long_press(void) {
    toggle_deep_sleep();
}

static void process_short_press(void) {
    toggle_recording_hit();
    esp_timer_stop(long_press_timer_handle);
}

/*
  Handle an interrupt triggered when a pad is touched.
  Recognize what pad has been touched and save it in a table.
 */
static void touchsensor_interrupt_cb(void *arg)
{
    int task_awoken = pdFALSE;
    touch_event_t evt;

    evt.intr_mask = touch_pad_read_intr_status_mask();
    evt.pad_status = touch_pad_get_status();

    xQueueSendFromISR(que_touch, &evt, &task_awoken);

    if (task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void tp_example_set_thresholds(void)
{
    uint32_t touch_value;
    //read benchmark value
    touch_pad_read_benchmark(TOUCH_PAD_NUM5, &touch_value);
    //set interrupt threshold.
    touch_pad_set_thresh(TOUCH_PAD_NUM5, touch_value * BUTTON_TRESHOLD);
}

static void touchsensor_filter_set(touch_filter_mode_t mode)
{
    /* Filter function */
    touch_filter_config_t filter_info = {
        .mode = mode,           // Test jitter and filter 1/4.
        .debounce_cnt = 1,      // 1 time count.
        .noise_thr = 0,         // 50%
        .jitter_step = 4,       // use for jitter mode.
        .smh_lvl = TOUCH_PAD_SMOOTH_IIR_2,
    };
    touch_pad_filter_set_config(&filter_info);
    touch_pad_filter_enable();
    ESP_LOGI(TAG, "touch pad filter init");
}

static void tp_example_read_task(void *pvParameter)
{
    touch_event_t evt = {0};
    /* Wait touch sensor init done */
    vTaskDelay(50 / portTICK_PERIOD_MS);
    tp_example_set_thresholds();

    while (1) {
        int ret = xQueueReceive(que_touch, &evt, (TickType_t)portMAX_DELAY);
        if (ret != pdTRUE) {
            continue;
        }
        if (evt.intr_mask & TOUCH_PAD_INTR_MASK_ACTIVE) {
            ESP_LOGI(TAG, "TouchSensor be activated, status mask 0x%lu", evt.pad_status);
            start_long_press_timer();
        }
        if (evt.intr_mask & TOUCH_PAD_INTR_MASK_INACTIVE) {
            if (is_long_press_timer_in_progress) {
                process_short_press();
            } else {
                process_short_press();
            }

            ESP_LOGI(TAG, "TouchSensor be inactivated, status mask 0x%lu", evt.pad_status);

        }
        if (evt.intr_mask & TOUCH_PAD_INTR_MASK_TIMEOUT) {
            /* Add your exception handling in here. */
            ESP_LOGI(TAG, "Touch sensor channel measure timeout. Skip this exception channel!!");
            touch_pad_timeout_resume(); // Point on the next channel to measure.
        }
    }
}

static void create_long_press_timer(void) {
    esp_timer_create_args_t timer_options = {
        .callback = long_press_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "long_press_timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_options, long_press_timer_handle));
}

void touch_control_init(void) {
    ESP_ERROR_CHECK(touch_pad_init());
    ESP_ERROR_CHECK(touch_pad_config(TOUCH_PAD_NUM5));

    touch_pad_denoise_t denoise = {
        /* The bits to be cancelled are determined according to the noise level. */
        .grade = TOUCH_PAD_DENOISE_BIT4,
        /* By adjusting the parameters, the reading of T0 should be approximated to the reading of the measured channel. */
        .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
    };
    touch_pad_denoise_set_config(&denoise);
    touch_pad_denoise_enable();
    
    /* Filter setting */
    touchsensor_filter_set(TOUCH_PAD_FILTER_IIR_16);
    touch_pad_timeout_set(true, SOC_TOUCH_PAD_THRESHOLD_MAX);
    /* Register touch interrupt ISR, enable intr type. */
    touch_pad_isr_register(touchsensor_interrupt_cb, NULL, TOUCH_PAD_INTR_MASK_ALL);
    /* If you have other touch algorithm, you can get the measured value after the `TOUCH_PAD_INTR_MASK_SCAN_DONE` interrupt is generated. */
    touch_pad_intr_enable(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_INACTIVE | TOUCH_PAD_INTR_MASK_TIMEOUT);

    /* Enable touch sensor clock. Work mode is "timer trigger". */
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_fsm_start();

    create_long_press_timer();

    // Start a task to show what pads have been touched
    xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 4096, NULL, 5, NULL);
}