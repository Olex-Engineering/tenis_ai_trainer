#include "driver/touch_pad.h"

#define LONG_PRESS_MICRO_SECONDS_AMOUNT     3000000 // 3 seconds
#define BUTTON_TRESHOLD                     0.2

typedef struct touch_msg {
    touch_pad_intr_mask_t intr_mask;
    uint32_t pad_status;
} touch_event_t;

void touch_control_init(void);