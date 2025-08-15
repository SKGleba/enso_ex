#ifndef __BOOTMGR_H__
#define __BOOTMGR_H__

#include <inttypes.h>
#include <stddef.h>

#include "bm_ext.h"
#include "bootstrap.h"
#include "fmgr.h"
#include "lv0.h"
#include "main.h"
#include "nskbl.h"
#include "paper.h"
#include "stage2.h"
#include "stage3.h"
#include "stor.h"
#include "utils.h"
#include "view.h"

#include "ff.h"

#define OS0_MOUNTIDX 2
#define OS0_MOUNTPATH "mnt2:"
#define UPDATESM_CUSTOM_PATH OS0_MOUNTPATH "/spl_ussm.self"
#define UPDATESM_DEFAULT_PATH OS0_MOUNTPATH "/sm/update_service_sm.self"
#define CFG_PATH OS0_MOUNTPATH "/lv0patch.cfg"

enum CMD_ENUMS {
    CMD_INVALID = 0,
    CMD_LIVEQUE,
    CMD_ERRBREAK,
    CMD_CSTART,
    CMD_LV0_KSP = CMD_CSTART,
    CMD_LV0_DAT,
    CMD_LV0_EXE,
    CMD_ARM_DAT,
    CMD_ARM_EXE,
    CMD_KBLPARM,
    CMD_COUNT
};

const char* valid_commands[CMD_COUNT] = {
    CFG_PATH,
    "LIVEQUE",
    "ERRBREAK",
    "LV0_KSP",
    "LV0_DAT",
    "LV0_EXE",
    "ARM_DAT",
    "ARM_EXE",
    "KBLPARM"
};

#endif // __BOOTMGR_H__