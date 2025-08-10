#ifndef __EXPORTS_H__
#define __EXPORTS_H__

#include "bm_ext.h"
#include "bootstrap.h"
#include "fmgr.h"
#include "main.h"
#include "nskbl.h"
#include "paper.h"
#include "stage2.h"
#include "stage3.h"
#include "stor.h"
#include "utils.h"
#include "view.h"
#include "lv0.h"

#define EXPORTS_VERSION 1
#define EXPORTS_MAGIC_1 0xE2E2E2E2
#define EXPORTS_MAGIC_2 0x2E2E2E2E

struct recovery_export_s {
    uint32_t magic[2];
    uint32_t version;
    uint8_t *session_id;
	struct eex_param_s *eex_param;
	ex_ports_struct *eex_ports;
    const volatile struct exports_bmx_s *bmx;
    const volatile struct exports_lbm_s *lbm;
    const volatile struct exports_fmgr_s *fmgr;
    const volatile struct exports_lv0_s *lv0;
    const volatile struct exports_main_s *main;
    const volatile struct exports_paper_s *paper;
    const volatile struct exports_stage2_s *stage2;
    const volatile struct exports_stage3_s *stage3;
    const volatile struct exports_stor_s *stor;
    const volatile struct exports_util_s *utils;
    const volatile struct exports_view_s *view;
    struct {
        struct exports_bmx_s bmx;
        struct exports_lbm_s lbm;
        struct exports_fmgr_s fmgr;
        struct exports_lv0_s lv0;
        struct exports_main_s main;
        struct exports_paper_s paper;
        struct exports_stage2_s stage2;
        struct exports_stage3_s stage3;
        struct exports_stor_s stor;
        struct exports_util_s utils;
        struct exports_view_s view;
    } d;
};

#endif // __EXPORTS_H__