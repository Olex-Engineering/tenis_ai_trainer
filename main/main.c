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
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

#include "main.h"
#include "ble/ble.h"
#include "storage/storage.h"
#include "esp_err.h"
#include "esp_pm.h"
#include "led/led.h"
#include "buzzer/buzzer.h"
#include "icm/icm.h"
#include "btr_gauge/btr_gauge.h"
#include "hit_recording_handler/hit_recording_handler.h"
#include "touch_control/touch_control.h"
#include "global_control/global_control.h"

void app_main(void)
{
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 80, // e.g. 80, 160, 240
        .min_freq_mhz = 40, // e.g. 40
        .light_sleep_enable = false, // enable light sleep
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
    init_storage();
    init_global_control();
    init_led();
    // init_ble();
    init_icm();
    init_btr_gauge();
    touch_control_init();

    #if IS_DATA_COLLECTOR_DEVICE
        init_hit_recording();
    #endif

    if (!IS_DATA_COLLECTOR_DEVICE) {
        icm_enable();
    }

    buzzer_init();
    play_power_on_sound();
}
