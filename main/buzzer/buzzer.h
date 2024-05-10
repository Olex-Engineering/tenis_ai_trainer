#include <stdint.h>

#define BUZZER_GPIO  38

typedef enum {
    PLAY_POWER_ON_SOUND,
    PLAY_POWER_OFF_SOUND,
    PLAY_GOOD_HIT_SOUND,
    PLAY_MEDIUM_HIT_SOUND,
    PLAY_BAD_HIT_SOUND
} buzzer_action_type_t;

typedef struct {
    buzzer_action_type_t type;
} buzzer_action_t;

typedef struct {
    uint8_t key;
    uint32_t duration;
} sound_params_t;

// Different sounds invoke functions

void play_power_on_sound(void);
void play_power_off_sound(void);
void play_good_hit_sound(void);
void play_medium_hit_sound(void);
void play_bad_hit_sound(void);

/**
 * Configure LEDC and run action task
*/
void buzzer_init(void);