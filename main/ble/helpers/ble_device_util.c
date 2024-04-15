#include <stdbool.h>
#include <string.h>

#include "esp_log.h"

#include "helpers/utils.h"

bool is_addresses_equals(uint8_t *addr1, uint8_t *addr2) {
    print_addr(addr1);
    print_addr(addr2);

    return memcmp(addr1, addr2, 6) == 0;
}