#ifndef __STAGE3_H__
#define __STAGE3_H__

#include "main.h"
#include "paper.h"
#include "../../../core/ex_defs.h"

#define S3_UDI_OUTFNAME "udi.bin"
#define S3_LEAF_CID 0x44
#define S3_LEAF_OPSID_0 0x46
#define S3_LEAF_OPSID_1 0x47

extern struct menu_s stage3_menu_s;
int stage3_menu(int selection);

#endif  // __STAGE3_H__