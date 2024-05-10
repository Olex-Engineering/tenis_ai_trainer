#include "main.h"

typedef enum {
    ACTION_DEEP_SLEEP_OFF,
    ACTION_DEEP_SLEEP_ON,

    #if IS_DATA_COLLECTOR_DEVICE
        ACTION_START_RECORDING_HIT,
        ACTION_END_RECORDING_HIT
    #endif
} global_action_type_t;

typedef struct {
    global_action_type_t type;
} global_action_t;

/**
 * Enable device deep sleep
 * Set action to queue
*/
void deep_sleep_on(void);
/**
 * Disable device deep sleep
 * Set action to queue
*/
void deep_sleep_off(void);

/**
 * Enable or disable device sleep depending on current state
*/
void toggle_deep_sleep(void);

#if IS_DATA_COLLECTOR_DEVICE
    /**
     * Start recording hit
     * Set action to queue
     * Enable icm
    */
    void start_recording_hit(void);
    /**
     * End recording hit
     * Set action to queue
     * Disable icm
     * Save data
    */
    void end_recording_hit(void);

    /**
     * Start or end recording single hit depending on current state
    */
    void toggle_recording_hit(void);
#endif

/**
 * Create queue and task
*/
void init_global_control(void);