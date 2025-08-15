
#include <inttypes.h>
#include <stddef.h>

#include "bootmgr.h"

static struct txtcfg_arg_s cmd_args[CMD_COUNT] = {
    [CMD_INVALID] = {0, 0, NULL, NULL, false},
    [CMD_LIVEQUE] = {4, 5, NULL, NULL, true},
    [CMD_ERRBREAK] = {4, 5, NULL, NULL, true},
    [CMD_LV0_KSP] = {14, 44, NULL, NULL, false}, // 0x0123,0x12,ABCDEF...
    [CMD_LV0_DAT] = {13, 8191, NULL, NULL, false}, // 0x01234567,ABCDEF...
    [CMD_LV0_EXE] = {13, 8191, NULL, NULL, false}, // 0x01234567,ABCDEF...
    [CMD_ARM_DAT] = {13, 8191, NULL, NULL, false}, // 0x01234567,ABCDEF...
    [CMD_ARM_EXE] = {13, 8191, NULL, NULL, false}, // 0x01234567,ABCDEF...
    [CMD_KBLPARM] = {7, 511, NULL, NULL, false}, // 0x01,ABCDEF...
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

int cmdh_allow_livexe(int idx, char *arg) {
    if (!arg || !*arg) {
        LOG("Invalid argument for LIVEQUE command\n");
        return -1;
    }
    LOG("LIVEQUE command parsed with arg: %s\n", arg);
    bool livexe = my_strncmp(arg, "true", 4) ? false : true;
    for (int i = CMD_CSTART; i < CMD_COUNT; i++) {
        if (cmd_args[i].cmd_handler) {
            LOG("cmd %d (%s) will %s execute upon parsing\n", i, valid_commands[i], livexe ? "now" : "not");
            cmd_args[i].exec = livexe;
        }
    }
    return 0;
}

int cmdh_breakproxy(int idx, char* arg) {
    if (idx == CMD_ERRBREAK) {
        if (!arg || !*arg) {
            LOG("Invalid argument for ERRBREAK command\n");
            return -1;
        }
        LOG("ERRBREAK command parsed with arg: %s\n", arg);
        if (!my_strncmp(arg, "true", 4)) {
            for (int i = CMD_CSTART; i < CMD_COUNT; i++) {
                if (cmd_args[i].cmd_handler) {
                    LOG("cmd %d (%s) will break further config exec on error\n", i, valid_commands[i]);
                    cmd_args[i].cmd_handler = cmdh_breakproxy;
                }
            }
        } else {
            LOG("Dispatching default command handlers\n");
            cmdh_dispatch_table(0);
        }
        return 0;
    }
    int (*actual_handler)(int, char *) = cmdh_dispatch_table(idx);
    if (!actual_handler) {
        LOG("No handler for command %d (%s)\n", idx, valid_commands[idx]);
        return -1;
    }
    int ret = actual_handler(idx, arg);
    if (ret < 0) {
        LOG("Error occurred while handling command %d (%s) - BREAK\n", idx, valid_commands[idx]);
        for (int i = CMD_CSTART; i < CMD_COUNT; i++)
            cmd_args[i].cmd_handler = NULL;
    }
    return ret;
}

int cmdh_ks(int idx, char* arg) {
    if (!arg || !*arg) {
        LOG("Invalid argument for KSP command\n");
        return -1;
    }
    LOG("KSP command parsed with arg: %s\n", arg);
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
        LOG("Failed to allocate memory for LV0_ARM_CID command\n");
        return -1;
    }
    memset(*pk, 0, sizeof(struct lv0p_k_s));
    char *argx = arg + 2; // 0x
    if (antoh(argx, (uint8_t*)&(*pk)->id, 2) >= 0) {
        (*pk)->id = BSWAP16((*pk)->id);
        argx += (4 + 1 + 2); // ABCD,0x
        if (antoh(argx, (uint8_t*)&(*pk)->off, 1) >= 0) {
            argx += (2 + 1); // EF,
            if (my_strncmp(argx, OS0_MOUNTPATH, strlen(OS0_MOUNTPATH))) {
                (*pk)->sz = strlen(argx) / 2;
                if (antoh(argx, (*pk)->data, (*pk)->sz) >= 0) {
                    LOG("Added KSP: 0x%08X @ 0x%08X [0x%08X] <= 0x%08X\n", (*pk)->id, (*pk)->off, (*pk)->sz, (*pk)->data);
                    return 0;
                } else
                    LOG("KSP: Failed to convert keyslot data\n");
            } else if (fmgr_get_file(argx, (*pk)->data, 0, 0, &(*pk)->sz)) {
                LOG("Added KSP: 0x%08X @ 0x%08X [0x%08X] <= %s\n", (*pk)->id, (*pk)->off, (*pk)->sz, argx);
                return 0;
            } else
                LOG("KSP: Failed to read file %s\n", argx);
        } else
            LOG("KSP: Failed to convert keyslot offset\n");
    } else
        LOG("KSP: Failed to convert keyslot id\n");
    my_free(*pk);
    *pk = NULL;
    return -1;
}

int cmdh_lv0dat(int idx, char* arg) {
    if (!arg || !*arg) {
        LOG("Invalid argument for LV0_DAT command\n");
        return -1;
    }
    LOG("LV0_DAT command parsed with arg: %s\n", arg);
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
    memset(*pd, 0, sizeof(struct lv0p_d_s));
    char *argx = arg + 2; // 0x
    if (antoh(argx, (uint8_t*)&(*pd)->dst, 4) >= 0) {
        (*pd)->dst = BSWAP32((*pd)->dst);
        argx += (8 + 1); // ABCDEF01,
        if (!my_strncmp(argx, "0x", 2)) {
            argx += 2; // 0x
            if (antoh(argx, (uint8_t*)&(*pd)->src, 4) >= 0) {
                (*pd)->src = BSWAP32((*pd)->src);
                argx += (8 + 1 + 2); // ABCDEF01,0x
                if (antoh(argx, (uint8_t*)&(*pd)->sz, 4) >= 0) {
                    (*pd)->sz = BSWAP32((*pd)->sz);
                    (*pd)->ncopyin = true;
                    LOG("Added LV0_DAT: 0x%08X @ 0x%08X => 0x%08X\n", (*pd)->sz, (*pd)->src, (*pd)->dst);
                    return 0;
                } else
                    LOG("LV0_DAT: Failed to convert size\n");
            } else
                LOG("LV0_DAT: Failed to convert src address\n");
        } else if (!my_strncmp(argx, OS0_MOUNTPATH, strlen(OS0_MOUNTPATH))) {
            (*pd)->src_va = fmgr_get_file(argx, NULL, 0, 0, &(*pd)->sz);
            if ((*pd)->src_va) {
                LOG("Added LV0_DAT: 0x%08X <= %s [0x%08X]\n", (*pd)->dst, argx, (*pd)->sz);
                return 0;
            } else
                LOG("LV0_DAT: Failed to read file %s\n", argx);
        } else {
            (*pd)->sz = strlen(argx) / 2;
            (*pd)->src_va = my_malloc((*pd)->sz);
            if ((*pd)->src_va) {
                if (antoh(argx, (*pd)->src_va, (*pd)->sz) >= 0) {
                    LOG("Added LV0_DAT: 0x%08X <= 0x%08X [0x%08X]\n", (*pd)->dst, (*pd)->src_va, (*pd)->sz);
                    return 0;
                } else
                    LOG("LV0_DAT: Failed to convert data\n");
                my_free((*pd)->src_va);
                (*pd)->src_va = NULL;
            } else
                LOG("LV0_DAT: Failed to allocate memory for data\n");
        }
    } else
        LOG("LV0_DAT: Failed to convert id\n");
    my_free(*pd);
    *pd = NULL;
    return -1;
}

int cmdh_lv0x(int idx, char *arg) {
    if (!arg || !*arg) {
        LOG("Invalid argument for LV0X command\n");
        return -1;
    }
    LOG("LV0X command parsed with arg: %s\n", arg);
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
        LOG("Failed to allocate memory for LV0X command\n");
        return -1;
    }
    memset(*px, 0, sizeof(struct lv0p_x_s));
    char *argx = arg + 2; // 0x
    if (antoh(argx, (uint8_t*)&(*px)->arg, 4) >= 0) {
        (*px)->arg = BSWAP32((*px)->arg);
        argx += (8 + 1); // ABCDEF01,
        if (!my_strncmp(argx, "0x", 2)) {
            argx += 2; // 0x
            if (antoh(argx, (uint8_t*)&(*px)->addr, 4) >= 0) {
                (*px)->addr = BSWAP32((*px)->addr);
                LOG("Added LV0X: 0x%08X(0x%08X)\n", (*px)->addr, (*px)->arg);
                return 0;
            } else
                LOG("LV0X: Failed to convert func paddress\n");
        } else if (!my_strncmp(argx, OS0_MOUNTPATH, strlen(OS0_MOUNTPATH))) {
            (*px)->src_va = fmgr_get_file(argx, NULL, 0, 0, &(*px)->c_sz);
            if ((*px)->src_va) {
                LOG("Added LV0X: 0x%08X @ %s(0x%08X)\n", (*px)->c_sz, argx, (*px)->arg);
                return 0;
            } else
                LOG("LV0X: Failed to read file %s\n", argx);
        } else {
            (*px)->c_sz = strlen(argx) / 2;
            (*px)->src_va = my_malloc((*px)->c_sz);
            if ((*px)->src_va) {
                if (antoh(argx, (*px)->src_va, (*px)->c_sz) >= 0) {
                    LOG("Added LV0X: %s(0x%08X)\n", argx, (*px)->arg);
                    return 0;
                } else
                    LOG("LV0X: Failed to convert data\n");
                my_free((*px)->src_va);
                (*px)->src_va = NULL;
            } else
                LOG("LV0X: Failed to allocate memory for data\n");
        }
    } else
        LOG("LV0X: Failed to convert arg\n");
    my_free(*px);
    *px = NULL;
    return -1;
}

int cmdh_kblp(int idx, char *arg) {
    if (!arg || !*arg) {
        LOG("Invalid argument for KBLPARAM command\n");
        return -1;
    }
    LOG("KBLPARAM command parsed with arg: %s\n", arg);
    uint8_t off = 0;
    char *argx = arg + 2; // 0x
    if (antoh(argx, &off, 1) >= 0) {
        argx += (2 + 1);
        uint8_t *kblp = g_eex_ports.kbl_param;
        if (my_strncmp(argx, OS0_MOUNTPATH, strlen(OS0_MOUNTPATH))) {
            int d_len = strlen(argx) / 2;
            LOG("KBLPARAM: %s -> 0x%08X [0x%02X]\n", argx, off, d_len);
            if (d_len) {
                if (antoh(argx, kblp + off, d_len) >= 0) {
                    LOG("Patched main kbl_param at offset 0x%02X with data: %s\n", off, argx);
                    kblp = (uint8_t *)(*(uint32_t *)(*(uint32_t *)(0x51138a3c) + 0x6c));
                    if (antoh(argx, kblp + off, d_len) >= 0) {
                        LOG("Patched sysrootNP2 kbl_param at offset 0x%02X with data: %s\n", off, argx);
                        return 0;
                    } else
                        LOG("KBLPARAM: Failed to convert data (NP2)\n");
                } else
                    LOG("KBLPARAM: Failed to convert data (MAIN)\n");
            } else
                LOG("KBLPARAM: No data to patch with\n");
        } else {
            uint32_t rb = 0;
            if (fmgr_get_file(argx, kblp + off, 0, 0, &rb)) {
                memcpy((uint8_t *)(*(uint32_t *)(*(uint32_t *)(0x51138a3c) + 0x6c)) + off, kblp + off, rb);
                LOG("Patched main & sysrootNP2 kbl_param at offset 0x%02X with file: %s [0x%08X]\n", off, argx, rb);
                return 0;
            } else
                LOG("KBLPARAM: Failed to read patch data from file %s\n", argx);
        }
    } else
        LOG("KBLPARAM: Failed to convert offset\n");
    return -1;
}

int cmdh_armd(int idx, char *arg) {
    if (!arg || !*arg) {
        LOG("Invalid argument for ARM D command\n");
        return -1;
    }
    LOG("ARM D command parsed with arg: %s\n", arg);
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
        LOG("Failed to allocate memory for ARM D command data\n");
        return -1;
    }
    memset(*pd, 0, sizeof(struct armp_d_s));
    char *argx = arg + 2; // 0x
    if (antoh(argx, (uint8_t *)&(*pd)->dst, 4) >= 0) {
        (*pd)->dst = (void *)BSWAP32((*pd)->dst);
        argx += (8 + 1); // ABCDEF01,
        if (!my_strncmp(argx, "0x", 2)) {
            argx += 2; // 0x
            if (antoh(argx, (uint8_t *)&(*pd)->src, 4) >= 0) {
                (*pd)->src = (void *)BSWAP32((*pd)->src);
                argx += (8 + 1 + 2); // ABCDEF01,0x
                if (antoh(argx, (uint8_t *)&(*pd)->sz, 4) >= 0) {
                    (*pd)->sz = BSWAP32((*pd)->sz);
                    LOG("Added ARM D: 0x%08X @ 0x%08X => 0x%08X\n", (*pd)->sz, (*pd)->src, (*pd)->sz);
                    return 0;
                } else
                    LOG("ARM D: Failed to convert size\n");
            } else
                LOG("ARM D: Failed to convert source address\n");
        } else if (!my_strncmp(argx, OS0_MOUNTPATH, strlen(OS0_MOUNTPATH))) {
            (*pd)->src = fmgr_get_file(argx, NULL, 0, 0, &(*pd)->sz);
            if ((*pd)->src) {
                (*pd)->src_fa = true;
                LOG("Added ARM D: 0x%08X @ %s => 0x%08X\n", (*pd)->sz, argx, (*pd)->dst);
                return 0;
            } else
                LOG("ARM D: Failed to read file %s\n", argx);
        } else {
            (*pd)->sz = strlen(argx) / 2;
            (*pd)->src = my_malloc((*pd)->sz);
            if ((*pd)->src) {
                if (antoh(argx, (*pd)->src, (*pd)->sz) >= 0) {
                    (*pd)->src_fa = true;
                    LOG("Added ARM D: 0x%08X @ %s => 0x%08X\n", (*pd)->sz, argx, (*pd)->dst);
                    return 0;
                } else
                    LOG("ARM D: Failed to convert data\n");
                my_free((*pd)->src);
                (*pd)->src = NULL;
            } else
                LOG("ARM D: Failed to allocate memory for data\n");
        }
    } else
        LOG("ARM D: Failed to convert destination address\n");
    my_free(*pd);
    *pd = NULL;
    return -1;
}

int cmdh_armx(int idx, char *arg) {
    if (!arg || !*arg) {
        LOG("Invalid argument for ARM X command\n");
        return -1;
    }
    LOG("ARM X command parsed with arg: %s\n", arg);
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
        LOG("Failed to allocate memory for ARM X command data\n");
        return -1;
    }
    memset(*px, 0, sizeof(struct armp_x_s));
    char *argx = arg + 2; // 0x
    if (antoh(argx, (uint8_t *)&(*px)->arg, 4) >= 0) {
        (*px)->arg = BSWAP32((*px)->arg);
        argx += (8 + 1); // ABCDEF01,
        if (!my_strncmp(argx, "0x", 2)) {
            argx += 2; // 0x
            if (antoh(argx, (uint8_t *)&(*px)->src, 4) >= 0) {
                (*px)->src = (void *)BSWAP32((*px)->src);
                LOG("Added ARM X: 0x%08X(0x%08X)\n", (*px)->src, (*px)->arg);
                return 0;
            } else
                LOG("ARM X: Failed to convert function address\n");
        } else if (!my_strncmp(argx, OS0_MOUNTPATH, strlen(OS0_MOUNTPATH))) {
            (*px)->src = fmgr_get_file(argx, NULL, 0, 0, &(*px)->c_sz);
            if ((*px)->src) {
                LOG("Added ARM X: 0x%08X @ %s(0x%08X)\n", (*px)->c_sz, argx, (*px)->arg);
                return 0;
            } else
                LOG("ARM X: Failed to read file %s\n", argx);
        } else {
            (*px)->c_sz = strlen(argx) / 2;
            (*px)->src = my_malloc((*px)->c_sz);
            if ((*px)->src) {
                if (antoh(argx, (*px)->src, (*px)->c_sz) >= 0) {
                    LOG("Added ARM X: 0x%08X @ %s(0x%08X)\n", (*px)->c_sz, argx, (*px)->arg);
                    return 0;
                } else
                    LOG("ARM X: Failed to convert data\n");
                my_free((*px)->src);
                (*px)->src = NULL;
            } else
                LOG("ARM X: Failed to allocate memory for data\n");
        }
    } else
        LOG("ARM X: Failed to convert address\n");
    my_free(*px);
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
        cmd_args[i].cmd_handler = dispatch_table[i];
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