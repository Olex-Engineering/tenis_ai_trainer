#include <stdint.h>

#define PREFERRED_MTU_VALUE     512
#define DEFAULT_DEVICE_NAME     "TenisTrainer"
#define BLE_TAG                 "BLE"

extern uint16_t conn_handle;

void init_ble(void);

void start_accepting_new_device_timer(void);

