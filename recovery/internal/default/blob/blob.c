#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "../../../../core/enso.h"

#define NO_DEBUG_LOGGING
#include "../../../../core/nskbl.h"

int _start(uint32_t get_info_va, uint32_t init_os0_va) {
    void (*init_os0)(int mbr_off) = (void*)init_os0_va;
    init_os0(ENSO_EMUMBR_OFFSET);
    return 1;
}