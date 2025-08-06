#ifndef __STAGE3_H__
#define __STAGE3_H__

#include "main.h"
#include "paper.h"
#include "../../../core/ex_defs.h"

#define STAGE3_UPR_BUFSIZE E2X_RBLOB_SIZE // nothing bigger than this
#define STAGE3_UPR_CONFIG_FNAME "rconfig.e2xr"
#define STAGE3_UPR_BLOB_FNAME "rblob.e2xp"
#define STAGE3_UPR_MBR_FNAME "rmbr.bin"

extern struct menu_s stage3_menu_s;
int stage3_menu(int selection);

#endif  // __STAGE3_H__