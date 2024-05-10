#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_err.h"
#include "driver/ledc.h"

#include "buzzer.h"

static QueueHandle_t sounds_queue;
static QueueHandle_t action_queue;

const uint16_t notes[]  = {
	0,31,33,35,37,39,41,44,46,49,52,55,58,62,65,69,73,78,82,87,93,98,104,110,117,123,131,139,147,156,165,175,185,196,208,220,
	233,247,262,277,294,311,330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047,1109,1175,1245,1319,1397,1480,1568,1661,
	1760,1865,1976,2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951,4186,4435,4699,4978 };

static void process_good_hit_sound(void) {
	sound_params_t params = {
		.key = 86,
		.duration = 1000
	};

	xQueueSendToBack(sounds_queue, &params, 50);
}

static void process_medium_hit_sound(void) {
	sound_params_t params = {
		.key = 70,
		.duration = 1000
	};

	xQueueSendToBack(sounds_queue, &params, 50);
}

static void process_bad_hit_sound(void) {
	sound_params_t params = {
		.key = 50,
		.duration = 1000
	};

	xQueueSendToBack(sounds_queue, &params, 50);
}

static void process_power_off_sound(void) {
	sound_params_t first_note_params = {
		.key = 85,
		.duration = 1000
	};

	sound_params_t second_note_params = {
		.key = 78,
		.duration = 500
	};

	xQueueSendToBack(sounds_queue, &first_note_params, 50);
	xQueueSendToBack(sounds_queue, &second_note_params, 50);
}

static void process_power_on_sound(void) {
	sound_params_t first_note_params = {
		.key = 78,
		.duration = 1000
	};

	sound_params_t second_note_params = {
		.key = 85,
		.duration = 500
	};

	xQueueSendToBack(sounds_queue, &first_note_params, 50);
	xQueueSendToBack(sounds_queue, &second_note_params, 50);
}


static void play_sound(sound_params_t *params) {
	ledc_timer_config_t ledc_timer;

	ledc_timer.duty_resolution = LEDC_TIMER_10_BIT;
	ledc_timer.freq_hz = notes[params->key];
	ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
	ledc_timer.timer_num = LEDC_TIMER_0;

	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

   	ledc_channel_config_t ledc_channel;
	
	ledc_channel.channel    = LEDC_CHANNEL_0;
	// TODO: find suitable duty for current buzzer
	ledc_channel.duty       = 0x7F;
	ledc_channel.gpio_num   = BUZZER_GPIO;
	ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
	ledc_channel.hpoint     = 0;
	ledc_channel.timer_sel  = LEDC_TIMER_0;

	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void stop_playing_sound() {
	ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
}

static void sound_handler_task(void *pvParams) {
	sound_params_t params;

	while (1) {
		if (xQueueReceive(sounds_queue, &params, portMAX_DELAY)) {
			play_sound(&params);
			vTaskDelay(params.duration / portTICK_PERIOD_MS);
			stop_playing_sound();
		}
	}
}

static void action_handler_task(void *pvParams) {
	buzzer_action_t action;

	while (1) {
		if (xQueueReceive(action_queue, &action, portMAX_DELAY)) {
			switch (action.type)
			{
			case PLAY_GOOD_HIT_SOUND:
				process_good_hit_sound();
				break;

			case PLAY_MEDIUM_HIT_SOUND:
				process_medium_hit_sound();
				break;

			case PLAY_BAD_HIT_SOUND:
				process_bad_hit_sound();
				break;

			case PLAY_POWER_ON_SOUND:
				process_power_on_sound();
				break;

			case PLAY_POWER_OFF_SOUND:
				process_power_off_sound();
				break;

			default:
				break;
			}
		}
	}
}

void play_good_hit_sound(void) {
	buzzer_action_t action = {
		.type = PLAY_GOOD_HIT_SOUND
	};

	xQueueSendToBack(action_queue, &action, 50);
}

void play_medium_hit_sound(void) {
    buzzer_action_t action = {
		.type = PLAY_MEDIUM_HIT_SOUND
	};

	xQueueSendToBack(action_queue, &action, 50);
}

void play_bad_hit_sound(void) {
    buzzer_action_t action = {
		.type = PLAY_BAD_HIT_SOUND
	};

	xQueueSendToBack(action_queue, &action, 50);
}

void play_power_on_sound(void) {
	buzzer_action_t action = {
		.type = PLAY_POWER_ON_SOUND
	};

	xQueueSendToBack(action_queue, &action, 50);
}

void play_power_off_sound(void) {
	buzzer_action_t action = {
		.type = PLAY_POWER_OFF_SOUND
	};

	xQueueSendToBack(action_queue, &action, 50);
}

void buzzer_init(void) {
	sounds_queue = xQueueCreate(4, sizeof(sound_params_t));
	action_queue = xQueueCreate(1, sizeof(buzzer_action_t));
	xTaskCreate(sound_handler_task, "sound_handler_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
	xTaskCreate(action_handler_task, "action_handler_task", configMINIMAL_STACK_SIZE, NULL, 7, NULL);
}