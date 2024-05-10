#include <stdint.h>

#define PORT 0
#define I2C_ADDR ICM42670_I2C_ADDR_GND

// Single ICM all axis (gyro + acc) length
#define ICM_DATA_LENGTH     sizeof(int16_t) * 6

// icm data in such order - AX, AY, AZ, GX, GY, GZ
typedef int16_t icm_data_t[6];

typedef enum {
    ICM_ACTION_ENABLE,
    ICM_ACTION_DISABLE,
} icm_action_type_t;

typedef struct {
    icm_action_type_t type;
} icm_action_t;

/**
 * Set default configuration for icm
 * Create queues and tasks
 * Must be called on device init
*/
void init_icm(void);
/**
 * Must be called on woke up or session start event
 * Resume freeRtos task
 * Add action to queue
*/
void icm_enable(void);
/**
 * Must be called on sleep or session end event
 * Suspend freeRtos task
 * Add action to queue
*/
void icm_disable(void);