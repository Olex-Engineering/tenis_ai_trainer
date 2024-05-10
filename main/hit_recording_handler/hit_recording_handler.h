#include <stdint.h>

#define MAX_SINGLE_HIT_DATA_LENGTH   ICM_DATA_LENGTH * 1000

extern QueueHandle_t icm_data_queue;

typedef enum {
    HIT_RECORDING_ACTION_SAVE_HIT,
    HIT_RECORDING_ACTION_ABORT_HIT,
} hit_recording_action_type_t;

typedef struct {
    hit_recording_action_type_t type;
} hit_recording_action_t;

/*
 * Put abort action task in action queue
*/
void hit_abort_recording(void);

/*
 * Put save action task in action queue
*/
void hit_save_recording(void);

/*
 * Create action queue, data task and action task
*/
void init_hit_recording(void);