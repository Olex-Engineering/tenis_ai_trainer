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

#include "ble/ble.h"
#include "storage/storage.h"
#include "esp_err.h"
#include "esp_pm.h"

void task_set_timer(void *param);

void task_set_timer(void *param) {
    struct timeval tv_set = {
        .tv_sec = 1711988732,
        .tv_usec = 0
    };
    settimeofday(&tv_set, NULL);

    // printf("Set time: %lld \n", (int64_t)tv_set.tv_sec);

    while (1) {
        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);
        // printf("Current time: %lld \n", (int64_t)tv_now.tv_sec);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    esp_pm_config_esp32_t pm_config = {
        .max_freq_mhz = 80, // e.g. 80, 160, 240
        .min_freq_mhz = 40, // e.g. 40
        .light_sleep_enable = true, // enable light sleep
    };
    ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
    init_storage();
    init_ble();
    xTaskCreate(task_set_timer, "task_set_time", 4096, NULL, 5, NULL);
}
