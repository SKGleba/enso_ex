
#include <inttypes.h>
#include <stddef.h>

#include "bootmgr.h"

static struct txtcfg_arg_s cmd_args[CMD_COUNT] = {
    [CMD_LIVEQUE] = {
        .arg = {
            [0] = {.min_len = 4, .max_len = 5},
        },
        .types = TXTCFG_TYPES_ALLOW(0, _ASCII),
        .exec = true
    },
    [CMD_ERRBREAK] = {
        .arg = {
            [0] = {.min_len = 4, .max_len = 5},
        },
        .types = TXTCFG_TYPES_ALLOW(0, _ASCII),
        .exec = true
    },
    [CMD_LV0_KSP] = {
        .arg = {
            [0] = {.min_len = 1, .max_len = 4}, // keyslot idx
            [1] = {.min_len = 1, .max_len = 0x20}, // patch offset or patch data
            [2] = {.min_len = 1, .max_len = 0x20}, // patch size or patch data
            [3] = {.min_len = 1, .max_len = 0x20}, // patch data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(2, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(3, _UINT, _RDATA, _FDATA)
    },
    [CMD_LV0_DAT] = {
        .arg = {
            [0] = {.min_len = 3, .max_len = 4}, // dst
            [1] = {.min_len = 1, .max_len = LV0_SPL_LV0P_BIG_MAXESIZE}, // sz or data
            [2] = {.min_len = 1, .max_len = LV0_SPL_LV0P_BIG_MAXESIZE}, // src or data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(2, _UINT, _RDATA, _FDATA)
    },
    [CMD_LV0_EXE] = {
        .arg = {
            [0] = {.min_len = 1, .max_len = LV0_SPL_LV0P_BIG_MAXESIZE}, // arg or data
            [1] = {.min_len = 1, .max_len = LV0_SPL_LV0P_BIG_MAXESIZE}, // addr or data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA)
    },
    [CMD_ARM_DAT] = {
        .arg = {
            [0] = {.min_len = 1, .max_len = 4}, // dst
            [1] = {.min_len = 1, .max_len = RMEMBLOCK_MAX_SIZE}, // sz or data
            [2] = {.min_len = 1, .max_len = RMEMBLOCK_MAX_SIZE}, // src or data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(2, _UINT, _RDATA, _FDATA)
    },
    [CMD_ARM_EXE] = {
        .arg = {
            [0] = {.min_len = 1, .max_len = RMEMBLOCK_MAX_SIZE}, // arg or data
            [1] = {.min_len = 1, .max_len = RMEMBLOCK_MAX_SIZE}, // addr or data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA)
    },
    [CMD_KBLPARM] = {
        .arg = {
            [0] = {.min_len = 1, .max_len = 4}, // offset
            [1] = {.min_len = 1, .max_len = 0x200}, // size or data
            [2] = {.min_len = 1, .max_len = 0x200}, // src or data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(2, _UINT, _RDATA, _FDATA)
    }
};

struct txtcfg_s txtcfg = {
    .buf = {
        .va = NULL,
        .size = 0
    },
    .args = {
        .count = CMD_COUNT,
        .names = valid_commands,
        .parsed = cmd_args
    }
};

struct lv0p_arg_s lv0p_args = {
    .magic = LV0P_ARG_MAGIC,
    .patcher = 0,
    .k = NULL,
    .d = NULL,
    .x = NULL
};

struct armp_arg_s armp_args = {
    .magic = ARMP_ARG_MAGIC,
    .d = NULL,
    .x = NULL
};

void *cmdh_dispatch_table(int idx);

int cmdh_allow_livexe(int idx, struct txtcfg_arg_s *arg) {
    if (!arg || !arg->handler) {
        LOG("ERROR: Invalid arguments for LIVEQUE command\n");
        return -1;
    }
    if (!(arg->types & TXTCFG_TYPES_PARSE(0, _ASCII))) {
        LOG("ERROR: Invalid argument type for LIVEQUE command\n");
        return -1;
    }
    bool livexe = my_strncmp(arg->arg[0].ascii, "true", 4) ? false : true;
    for (int i = CMD_CSTART; i < CMD_COUNT; i++) {
        if (cmd_args[i].handler) {
            LOG("cmd %d (%s) will %s execute upon parsing\n", i, valid_commands[i], livexe ? "now" : "not");
            cmd_args[i].exec = livexe;
        }
    }
    return 0;
}

int cmdh_breakproxy(int idx, struct txtcfg_arg_s *arg) {
    if (idx == CMD_ERRBREAK) {
        if (!arg || !arg->handler) {
            LOG("ERROR: Invalid argument for ERRBREAK command\n");
            return -1;
        }
        if (!(arg->types & TXTCFG_TYPES_PARSE(0, _ASCII))) {
            LOG("ERROR: Invalid argument type for ERRBREAK command\n");
            return -1;
        }
        if (!my_strncmp(arg->arg[0].ascii, "true", 4)) {
            for (int i = CMD_CSTART; i < CMD_COUNT; i++) {
                if (cmd_args[i].handler) {
                    LOG("cmd %d (%s) will break further config exec on error\n", i, valid_commands[i]);
                    cmd_args[i].handler = cmdh_breakproxy;
                }
            }
        } else {
            LOG("Dispatching default command handlers\n");
            cmdh_dispatch_table(0);
        }
        return 0;
    }
    int (*actual_handler)(int, struct txtcfg_arg_s *) = cmdh_dispatch_table(idx);
    if (!actual_handler) {
        LOG("ERROR: No handler for command %d (%s)\n", idx, valid_commands[idx]);
        return -1;
    }
    int ret = actual_handler(idx, arg);
    if (ret < 0) {
        LOG("ERROR: Error occurred while handling command %d (%s) - BREAK\n", idx, valid_commands[idx]);
        for (int i = CMD_CSTART; i < CMD_COUNT; i++)
            cmd_args[i].handler = NULL;
    }
    return ret;
}

int cmdh_ks(int idx, struct txtcfg_arg_s *arg) {
    if (!arg || !arg->handler) {
        LOG("ERROR: Invalid argument for KSP command\n");
        return -1;
    }
    struct lv0p_k_s **pk = NULL;
    struct lv0p_k_s *k = lv0p_args.k;
    while (k) {
        pk = &k->next;
        k = k->next;
    }
    if (!pk)
        pk = &lv0p_args.k;
    *pk = my_malloc(sizeof(struct lv0p_k_s));
    if (!*pk) {
        LOG("ERROR: Failed to allocate memory for LV0_ARM_CID command\n");
        return -1;
    }
    k = *pk;
    memset(k, 0, sizeof(struct lv0p_k_s));
    int ai = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        k->id = arg->arg[ai].uintgr;
        ai++;
        if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
            k->off = arg->arg[ai].uintgr;
            ai++;
        }
        if ((ai == 2) && (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT))) {
            k->sz = arg->arg[ai].uintgr;
            ai++;
        }
        if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA)) && arg->arg[ai].act_len) {
            if (ai == 1)
                k->off = 0;
            else if (ai == 2)
                k->sz = arg->arg[ai].act_len;
            if (k->sz <= sizeof(k->data)) {
                memcpy(k->data, arg->arg[ai].data, k->sz);
                LOG("Added KSP: [0x%08X] => 0x%08X @ 0x%08X\n", k->sz, k->id, k->off);
                return 0;
            } else
                LOG("ERROR: KSP patch data too large\n");
        } else
            LOG("ERROR: Invalid KSP data arg\n");
    } else
        LOG("ERROR: Invalid keyslot index type\n");
    my_free(k);
    *pk = NULL;
    return -1;
}

int cmdh_lv0dat(int idx, struct txtcfg_arg_s *arg) {
    if (!arg || !arg->handler) {
        LOG("ERROR: Invalid argument for LV0_DAT command\n");
        return -1;
    }
    struct lv0p_d_s **pd = NULL;
    struct lv0p_d_s *d = lv0p_args.d;
    while (d) {
        pd = &d->next;
        d = d->next;
    }
    if (!pd)
        pd = &lv0p_args.d;
    *pd = my_malloc(sizeof(struct lv0p_d_s));
    if (!*pd) {
        LOG("Failed to allocate memory for LV0_DAT command\n");
        return -1;
    }
    d = *pd;
    memset(d, 0, sizeof(struct lv0p_d_s));
    int ai = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        d->dst = arg->arg[ai].uintgr;
        ai++;
        if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
            d->sz = arg->arg[ai].uintgr;
            ai++;
            if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
                d->src = arg->arg[ai].uintgr;
                d->ncopyin = true;
                LOG("Added LV0_DAT: [0x%08X] 0x%08X => 0x%08X\n", d->sz, d->src, d->dst);
                return 0;
            }
        }
        if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA)) && arg->arg[ai].act_len) {
            if (ai == 1)
                d->sz = arg->arg[ai].act_len;
            d->src_va = arg->arg[ai].data;
            arg->arg[ai].data = NULL; // transfer ownership to the runner
            LOG("Added LV0_DAT: [0x%08X] => 0x%08X\n", d->sz, d->dst);
            return 0;
        } else
            LOG("ERROR: Invalid LV0_DAT src arg\n");
    } else
        LOG("ERROR: Invalid LV0_DAT dest arg\n");
    my_free(d);
    *pd = NULL;
    return -1;
}

int cmdh_lv0x(int idx, struct txtcfg_arg_s *arg) {
    if (!arg || !arg->handler) {
        LOG("ERROR: Invalid argument for LV0X command\n");
        return -1;
    }
    struct lv0p_x_s **px = NULL;
    struct lv0p_x_s *x = lv0p_args.x;
    while (x) {
        px = &x->next;
        x = x->next;
    }
    if (!px)
        px = &lv0p_args.x;
    *px = my_malloc(sizeof(struct lv0p_x_s));
    if (!*px) {
        LOG("ERROR: Failed to allocate memory for LV0X command\n");
        return -1;
    }
    x = *px;
    memset(x, 0, sizeof(struct lv0p_x_s));
    int ai = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        x->arg = arg->arg[ai].uintgr;
        ai++;
        if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
            x->addr = arg->arg[ai].uintgr;
            LOG("Added LV0X: 0x%08X(0x%08X)\n", x->addr, x->arg);
            return 0;
        }
    }
    if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA)) && arg->arg[ai].act_len) {
        x->c_sz = arg->arg[ai].act_len;
        x->src_va = arg->arg[ai].data;
        arg->arg[ai].data = NULL;
        LOG("Added LV0X: dyn(0x%08X)\n", x->arg);
        return 0;
    } else
        LOG("ERROR: Invalid LV0X src arg\n");
    my_free(x);
    *px = NULL;
    return -1;
}

int cmdh_kblp(int idx, struct txtcfg_arg_s *arg) {
    if (!arg || !arg->handler) {
        LOG("ERROR: Invalid argument for KBLPARAM command\n");
        return -1;
    }
    uint8_t *kblp1 = g_eex_ports.kbl_param;
    uint8_t *kblp2 = (uint8_t *)(*(uint32_t *)(*(uint32_t *)(0x51138a3c) + 0x6c));
    if (!(arg->types & TXTCFG_TYPES_PARSE(0, _UINT))) {
        LOG("ERROR: Invalid KBLP offset argument\n");
        return -1;
    }
    int ai = 1;
    uint32_t size = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        size = arg->arg[ai].uintgr;
        if (!size) {
            LOG("ERROR: Invalid KBLP size argument\n");
            return -1;
        }
        ai++;
    }
    if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA, _UINT)) && arg->arg[ai].act_len) {
        if (!size)
            size = arg->arg[ai].act_len;
        memcpy(kblp1 + arg->arg[0].uintgr, arg->arg[ai].data, size);
        LOG("Patched kbl_param at offset 0x%02X with data from 0x%08X\n", arg->arg[0].uintgr, arg->arg[ai].data);
        memcpy(kblp2 + arg->arg[0].uintgr, arg->arg[ai].data, size);
        LOG("Patched kbl_param NP2 at offset 0x%02X with data from 0x%08X\n", arg->arg[0].uintgr, arg->arg[ai].data);
        return 0;
    } else
        LOG("ERROR: Invalid KBLP src arg\n");
    return -1;
}

int cmdh_armd(int idx, struct txtcfg_arg_s *arg) {
    if (!arg || !arg->handler) {
        LOG("ERROR: Invalid argument for ARM D command\n");
        return -1;
    }
    struct armp_d_s **pd = NULL;
    struct armp_d_s *d = armp_args.d;
    while (d) {
        pd = &d->next;
        d = d->next;
    }
    if (!pd)
        pd = &armp_args.d;
    *pd = my_malloc(sizeof(struct armp_d_s));
    if (!*pd) {
        LOG("ERROR: Failed to allocate memory for ARM D command data\n");
        return -1;
    }
    d = *pd;
    memset(d, 0, sizeof(struct armp_d_s));
    int ai = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        d->dst = arg->arg[ai].data;
        ai++;
        if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
            d->sz = arg->arg[ai].uintgr;
            ai++;
            if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
                d->src = arg->arg[ai].data;
                LOG("Added ARM_DAT: [0x%08X] 0x%08X => 0x%08X\n", d->sz, d->src, d->dst);
                return 0;
            }
        }
        if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA)) && arg->arg[ai].act_len) {
            if (ai == 1)
                d->sz = arg->arg[ai].act_len;
            d->src = arg->arg[ai].data;
            arg->arg[ai].data = NULL;
            d->src_fa = true; // indicate that the source is a freeable address
            LOG("Added ARM_DAT: [0x%08X] => 0x%08X\n", d->sz, d->dst);
            return 0;
        } else
            LOG("ERROR: Invalid ARM_DAT src arg\n");
    } else
        LOG("ERROR: Invalid ARM_DAT dest arg\n");
    my_free(d);
    *pd = NULL;
    return -1;
}

int cmdh_armx(int idx, struct txtcfg_arg_s *arg) {
    if (!arg || !arg->handler) {
        LOG("ERROR: Invalid argument for ARM X command\n");
        return -1;
    }
    struct armp_x_s **px = NULL;
    struct armp_x_s *x = armp_args.x;
    while (x) {
        px = &x->next;
        x = x->next;
    }
    if (!px)
        px = &armp_args.x;
    *px = my_malloc(sizeof(struct armp_x_s));
    if (!*px) {
        LOG("ERROR: Failed to allocate memory for ARM X command data\n");
        return -1;
    }
    x = *px;
    memset(x, 0, sizeof(struct armp_x_s));
    int ai = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        x->arg = arg->arg[ai].uintgr;
        ai++;
        if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
            x->src = arg->arg[ai].data;
            LOG("Added ARM X: 0x%08X(0x%08X)\n", x->src, x->arg);
            return 0;
        }
    }
    if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA)) && arg->arg[ai].act_len) {
        x->c_sz = arg->arg[ai].act_len;
        x->src = arg->arg[ai].data;
        arg->arg[ai].data = NULL;
        LOG("Added ARM X: dyn(0x%08X)\n", x->arg);
        return 0;
    } else
        LOG("ERROR: Invalid ARM X src arg\n");
    my_free(x);
    *px = NULL;
    return -1;
}

void *cmdh_dispatch_table(int idx) {
    void *dispatch_table[CMD_COUNT] = {
        NULL,
        cmdh_allow_livexe,
        cmdh_breakproxy,
        cmdh_ks,
        cmdh_lv0dat,
        cmdh_lv0x,
        cmdh_armd,
        cmdh_armx,
        cmdh_kblp,
    };
    if (idx)
        return dispatch_table[idx];
    for (int i = 0; i < CMD_COUNT; i++)
        cmd_args[i].handler = dispatch_table[i];
    return NULL;
}

static int b_lv0spoof(void) {
    cmdh_dispatch_table(0); // Initialize command handlers
    int ret = txtcfg_loadExec(&txtcfg, true);
    if (ret >= 0) {
        if (lv0p_args.k || lv0p_args.d || lv0p_args.x) {
            LOG("Starting BIG lv0p_run with args: k=0x%08X, d=0x%08X, x=0x%08X\n", (unsigned int)lv0p_args.k, (unsigned int)lv0p_args.d, (unsigned int)lv0p_args.x);
            ret = lv0p_run(&lv0p_args, LV0_SPL_LV0P_BIG_ADDR, LV0_SPL_LV0P_BIG_SIZE, CHAIN_FREE_NESTED);
            LOG("lv0p_run returned: 0x%08X\n", ret);
            void *frbuf = NULL;
            struct lv0p_k_s *k = lv0p_args.k;
            while(k) {
                LOG("LVOP K(0x%08X @ 0x%08X <= 0x%08X) ret 0x%08X\n", k->off, k->id, k->sz, k->ret);
                frbuf = (void *)k;
                k = k->next;
                my_free(frbuf);
            }
            struct lv0p_d_s *d = lv0p_args.d;
            while(d) {
                LOG("LVOP D(0x%08X @ 0x%08X => 0x%08X) ret 0x%08X\n", d->sz, d->src, d->dst, d->ret);
                frbuf = (void *)d;
                d = d->next;
                my_free(frbuf);
            }
            struct lv0p_x_s *x = lv0p_args.x;
            while(x) {
                LOG("LVOP X(0x%08X(0x%08X)) ret 0x%08X\n", x->addr, x->arg, x->ret);
                frbuf = (void *)x;
                x = x->next;
                my_free(frbuf);
            }
        }
        if (armp_args.d || armp_args.x) {
            LOG("Starting armp_run with args: d=0x%08X, x=0x%08X\n", (unsigned int)armp_args.d, (unsigned int)armp_args.x);
            ret = armp_run(&armp_args, CHAIN_FREE_NESTED);
            LOG("armp_run returned: 0x%08X\n", ret);
            void *frbuf = NULL;
            struct armp_d_s *d = armp_args.d;
            while(d) {
                LOG("ARMP D(0x%08X @ 0x%08X => 0x%08X) ret 0x%08X\n", d->sz, d->src, d->dst, d->ret);
                frbuf = (void *)d;
                d = d->next;
                my_free(frbuf);
            }
            struct armp_x_s *x = armp_args.x;
            while(x) {
                LOG("ARMP X(0x%08X(0x%08X)) ret 0x%08X\n", x->src, x->arg, x->ret);
                frbuf = (void *)x;
                x = x->next;
                my_free(frbuf);
            }
        }
    }
    return 0;
}

static int b_initSPL_run(int (*payloader)(void)) {
    LOG("Mounting active os0...\n");
    uint32_t os0_nfo = fmgr_get_nskbl_os0();
    if (os0_nfo == 0xFFFFFFFF) {
        LOG("No os0 mounted by nskbl, trying %s one\n", IS_GCSD_INITIALIZED() ? "GCSD" : "default");
        if (IS_GCSD_INITIALIZED())
            os0_nfo = FMGR_MASTER_SCAN_PACK(MOUNT_MASTER_GCSD, STOR_PART_ENTIRE, STOR_PART_ACTIVE_BOTH);
        else
            os0_nfo = FMGR_MASTER_SCAN_PACK(MOUNT_MASTER_EMMC, STOR_PART_OS, STOR_PART_ACTIVE_YES);
    }
    if (stor_init_master(FMGR_MASTER_SCAN_UNPACK(MASTER, os0_nfo)) != MOUNT_MASTER_TYPE_NONE) {
        if (stor_init_mount(OS0_MOUNTIDX, FMGR_MASTER_SCAN_UNPACK(MASTER, os0_nfo), FMGR_MASTER_SCAN_UNPACK(PARTITION, os0_nfo), FMGR_MASTER_SCAN_UNPACK(ACTIVE, os0_nfo)) >= 0) {
            if (fmgr_mount(true, OS0_MOUNTIDX) == FR_OK) {
                LOG("Initializing SPL...\n");
                if (lv0_init(fmgr_get_file_size(UPDATESM_CUSTOM_PATH) ? UPDATESM_CUSTOM_PATH : UPDATESM_DEFAULT_PATH) >= 0)
                    return payloader();
                else
                    LOG("Failed to initialize SPL\n");
                fmgr_mount(false, OS0_MOUNTIDX);
            } else
                LOG("Failed to mount active os0 (FATFS)\n");
            stor_umount(OS0_MOUNTIDX);
        } else
            LOG("Failed to mount active os0 (STOR)\n");
    } else
        LOG("Failed to initialize os0 master\n");
    return -1;
}

__attribute__((section(".text.bootstart"))) int bootstart(void) {
	struct eex_param_s tmp_eex_params = {
            .boot_mode = BOOTSTRAP_MODE_LIB, // _BOOTMGR will autoinit the display and require calling deinit() on exit
            .get_hwcfg_patched = (int (*)(patchedHwcfgStruct *))(*(uint32_t*)NSKBL_EXPORTS(NSKBL_EXPORTS_GET_HWCFG_N)),
            .stage2_config = NULL
        };
    if (init(&tmp_eex_params) < 0) {
        nskbl_printf("Failed to initialize e2xr in LIB mode\n");
        return -1;
    }
    LOG("b_initSPL_run returned: 0x%08X\n", b_initSPL_run(b_lv0spoof));
    deinit();
    return E2X_EXE_RET_NORESIDENT; // indicate that we can be freed
}