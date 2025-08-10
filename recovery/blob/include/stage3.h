#ifndef __STAGE3_H__
#define __STAGE3_H__

#include "main.h"
#include "paper.h"
#include "../../../core/ex_defs.h"

#define S3_UDI_OUTFNAME "udi.bin"
#define S3_LEAF_CID 0x44
#define S3_LEAF_OPSID_0 0x46
#define S3_LEAF_OPSID_1 0x47

#ifndef RXP_PIE
extern struct menu_s stage3_menu_s;
int stage3_menu(int selection);
#else
#define r_stage3_menu(...) _r->stage3->stage3_menu(__VA_ARGS__)
#define r_stage3_menu_s (_r->stage3->stage3_menu_s)
#endif

struct exports_stage3_s {
    int (*stage3_menu)(int selection);
    struct menu_s *stage3_menu_s;
};

#endif  // __STAGE3_H__