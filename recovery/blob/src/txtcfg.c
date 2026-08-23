#include "lv0.h"
#include "txtcfg.h"

struct txtcfg_arg_s cmdh_args[CMDH_DCOUNT] = {
    [CMDH_LIVEQUE] = {
		.name = "LIVEQUE",
        .uarg = {
            [0] = {.min_len = 4, .max_len = 5},
        },
        .types = TXTCFG_TYPES_ALLOW(0, _ASCII),
        .exec = true
    },
    [CMDH_ERRBREAK] = {
		.name = "ERRBREAK",
        .uarg = {
            [0] = {.min_len = 4, .max_len = 5},
        },
        .types = TXTCFG_TYPES_ALLOW(0, _ASCII),
        .exec = true
    },
    [CMDH_MNTINIT] = {
		.name = "MNTINIT",
        .uarg = {
            [0] = {.min_len = 1, .max_len = 8}, // mount idx
            [1] = {.min_len = 4, .max_len = 8}, // stor master
            [2] = {.min_len = 1, .max_len = 8}, // stor partition id
            [3] = {.min_len = 1, .max_len = 8}, // stor active
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT, _ASCII) | TXTCFG_TYPES_ALLOW(1, _UINT, _ASCII) | TXTCFG_TYPES_ALLOW(2, _UINT, _ASCII) | TXTCFG_TYPES_ALLOW(3, _UINT, _ASCII),
    },
    [CMDH_LV0INIT] = {
		.name = "LV0INIT",
        .uarg = {
            [0] = {.min_len = 4, .max_len = 255}, // update_sm path
        },
        .types = TXTCFG_TYPES_ALLOW(0, _ASCII),
    },
    [CMDH_LV0STCK] = {
		.name = "LV0STCK",
        .uarg = {
            [0] = {.min_len = 1, .max_len = 4}, // stack ptr
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT),
    },
    [CMDH_LV0_KSP] = {
		.name = "LV0_KSP",
        .uarg = {
            [0] = {.min_len = 1, .max_len = 4}, // keyslot idx
            [1] = {.min_len = 1, .max_len = 0x20}, // patch offset or patch data
            [2] = {.min_len = 1, .max_len = 0x20}, // patch size or patch data
            [3] = {.min_len = 1, .max_len = 0x20}, // patch data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(2, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(3, _UINT, _RDATA, _FDATA)
    },
    [CMDH_LV0_DAT] = {
		.name = "LV0_DAT",
        .uarg = {
            [0] = {.min_len = 3, .max_len = 4}, // dst
            [1] = {.min_len = 1, .max_len = LV0_SPL_LV0P_BIG_MAXESIZE}, // sz or data
            [2] = {.min_len = 1, .max_len = LV0_SPL_LV0P_BIG_MAXESIZE}, // src or data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(2, _UINT, _RDATA, _FDATA)
    },
    [CMDH_LV0_EXE] = {
		.name = "LV0_EXE",
        .uarg = {
            [0] = {.min_len = 1, .max_len = LV0_SPL_LV0P_BIG_MAXESIZE}, // arg or data
            [1] = {.min_len = 1, .max_len = LV0_SPL_LV0P_BIG_MAXESIZE}, // addr or data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA)
    },
    [CMDH_ARM_DAT] = {
		.name = "ARM_DAT",
        .uarg = {
            [0] = {.min_len = 1, .max_len = 4}, // dst
            [1] = {.min_len = 1, .max_len = RMEMBLOCK_MAX_SIZE}, // sz or data
            [2] = {.min_len = 1, .max_len = RMEMBLOCK_MAX_SIZE}, // src or data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(2, _UINT, _RDATA, _FDATA)
    },
    [CMDH_ARM_EXE] = {
		.name = "ARM_EXE",
        .uarg = {
            [0] = {.min_len = 1, .max_len = RMEMBLOCK_MAX_SIZE}, // arg or data
            [1] = {.min_len = 1, .max_len = RMEMBLOCK_MAX_SIZE}, // addr or data
            [2] = {.min_len = 1, .max_len = RMEMBLOCK_MAX_SIZE}, // arg2 - ptr
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(2, _UINT)
    },
    [CMDH_KBLPARM] = {
		.name = "KBLPARM",
        .uarg = {
            [0] = {.min_len = 1, .max_len = 4}, // offset
            [1] = {.min_len = 1, .max_len = 0x200}, // size or data
            [2] = {.min_len = 1, .max_len = 0x200}, // src or data
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _RDATA, _FDATA) | TXTCFG_TYPES_ALLOW(2, _UINT, _RDATA, _FDATA)
    },
    [CMDH_M_ALLOC] = {
		.name = "M_ALLOC",
        .uarg = {
            [0] = {.min_len = 1, .max_len = 4}, // alias/overlay addr
            [1] = {.min_len = 1, .max_len = 4}, // type (uint or "rx/rw/uc")
            [2] = {.min_len = 1, .max_len = 255}, // size or file path (read on cmd exec)
            [3] = {.min_len = 1, .max_len = 4}, // paddr (for PA allocations)
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _ASCII) | TXTCFG_TYPES_ALLOW(2, _UINT, _ASCII) | TXTCFG_TYPES_ALLOW(3, _UINT)
    },
    [CMDH_M_FREE] = {
		.name = "M_FREE",
        .uarg = {
            [0] = {.min_len = 1, .max_len = 4}, // alias/overlay addr
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT)
    },
    [CMDH_M_RMAP] = {
		.name = "M_RMAP",
        .uarg = {
            [0] = {.min_len = 1, .max_len = 4}, // alias/overlay addr
            [1] = {.min_len = 1, .max_len = 4}, // type (uint or "rx/rw/uc")
        },
        .types = TXTCFG_TYPES_ALLOW(0, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _ASCII)
    },
    [CMDH_DOPATCH] = {
        .name = "_PATCH_",
        .uarg = {
            [0] = {.min_len = 3, .max_len = 4}, // chain - "all"/"arm"/"lv0"
            [1] = {.min_len = 1, .max_len = 4}, // cleanup - 0/1 or "yes"/"no"
        },
        .types = TXTCFG_TYPES_ALLOW(0, _ASCII)
    },
    [CMDH_FILERW] = {
        .name = "FILERW",
        .uarg = {
            [0] = {.min_len = 1, .max_len = 255}, // dst (addr or path)
            [1] = {.min_len = 1, .max_len = 255}, // src (addr or path-fdata)
            [2] = {.min_len = 1, .max_len = 4}, // size
        },
        .types = TXTCFG_TYPES_ALLOW(0, _ASCII, _UINT) | TXTCFG_TYPES_ALLOW(1, _UINT, _ASCII) | TXTCFG_TYPES_ALLOW(2, _UINT)
    }
};

struct lv0p_arg_s cmdh_lv0p_args = {.magic = LV0P_ARG_MAGIC, .patcher = 0, .stack = 0, .k = NULL, .d = NULL, .x = NULL};
struct armp_arg_s cmdh_armp_args = {.magic = ARMP_ARG_MAGIC, .d = NULL, .x = NULL};

static void *cmdh_get_valias(struct txtcfg_s *cfg, uint32_t alias, bool never_null) {
    for (int i = 0; i < TXTCFG_MAX_OVERLAYS; i++) {
        if (cfg->overlay[i].size == 0)
            continue;
        if ((alias >= cfg->overlay[i].alias) && (alias < (cfg->overlay[i].alias + cfg->overlay[i].size))) {
            DLOG("Found alias 0x%08X in overlay %d (0x%08X - 0x%08X)\n", alias, i, cfg->overlay[i].alias, cfg->overlay[i].alias + cfg->overlay[i].size);
            return (void *)((uint32_t)cfg->overlay[i].va + (alias - cfg->overlay[i].alias));
        }
    }
    if (never_null)
        return (void *)alias; // if not found, treat alias as a direct address
    return NULL;
}

int cmdh_allow_livexe(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!cfg || !cfg->arg || !cfg->arg[idx].handler || (arg != &cfg->arg[idx])) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    if (!(arg->types & TXTCFG_TYPES_PARSE(0, _ASCII))) {
        ELOG("Invalid argument type for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    bool livexe = my_strncmp(arg->uarg[0].ascii, "true", 4) ? false : true;
    for (int i = cfg->arg_hardc; i < cfg->arg_count; i++) {
        if (cfg->arg[i].handler) {
            DLOG("cmd %d (%s) will %s execute upon parsing\n", i, cfg->arg[i].name, livexe ? "now" : "not");
            cfg->arg[i].exec = livexe;
        }
    }
    return 0;
}

int cmdh_breakproxy(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (idx == CMDH_ERRBREAK) {
        if (!cfg || !cfg->arg || !cfg->arg[idx].handler || (arg != &cfg->arg[idx])) {
            ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
            return -1;
        }
        if (!(arg->types & TXTCFG_TYPES_PARSE(0, _ASCII))) {
            ELOG("Invalid argument type for command %s\n", cmdh_args[idx].name);
            return -1;
        }
        if (!my_strncmp(arg->uarg[0].ascii, "true", 4)) {
            for (int i = cfg->arg_hardc; i < cfg->arg_count; i++) {
                if (cfg->arg[i].handler) {
                    DLOG("cmd %d (%s) will break further config exec on error\n", i, cfg->arg[i].name);
                    cfg->arg[i].handler = cmdh_breakproxy;
                }
            }
            cfg->brhandler = cmdh_breakproxy;
        } else {
            DLOG("Dispatching default command handlers\n");
            cfg->h_dispatcher(0);
			cfg->brhandler = NULL;
        }
        return 0;
    }
	int ret = 0xDEADBEEF; // heh
	if (arg) {
    	int (*actual_handler)(int, struct txtcfg_arg_s *, struct txtcfg_s *) = cfg->h_dispatcher(idx);
    	if (!actual_handler) {
        	ELOG("No handler for command %d (%s)\n", idx, cfg->arg[idx].name);
        	return -1;
    	}
    	ret = actual_handler(idx, arg, cfg);
	}
    if (ret < 0) {
        ELOG("Error occurred while %s command %d (%s): 0x%08X - BREAK\n", arg ? "handling" : "parsing args of", idx, cfg->arg[idx].name, ret);
        for (int i = 0; i < cfg->arg_count; i++)
            cfg->arg[i].handler = NULL;
        //cfg->brhandler = NULL;
        cfg->dead = true;
    }
    return ret;
}

int cmdh_lv0stck(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    if (!(arg->types & TXTCFG_TYPES_PARSE(0, _UINT))) {
        ELOG("Invalid argument type for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    cmdh_lv0p_args.stack = arg->uarg[0].uintgr;
    DLOG("LV0 pchain runner stack set to 0x%08X\n", cmdh_lv0p_args.stack);
    return 0;
}

int cmdh_ks(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    struct lv0p_k_s **pk = NULL;
    struct lv0p_k_s *k = cmdh_lv0p_args.k;
    while (k) {
        pk = &k->next;
        k = k->next;
    }
    if (!pk)
        pk = &cmdh_lv0p_args.k;
    *pk = my_malloc(sizeof(struct lv0p_k_s));
    if (!*pk) {
        ELOG("Failed to allocate memory for LV0_ARM_CID command\n");
        return -1;
    }
    k = *pk;
    memset(k, 0, sizeof(struct lv0p_k_s));
    int ai = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        k->id = arg->uarg[ai].uintgr;
        ai++;
        if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
            k->off = arg->uarg[ai].uintgr;
            ai++;
        }
        if ((ai == 2) && (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT))) {
            k->sz = arg->uarg[ai].uintgr;
            ai++;
        }
        if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA)) && arg->uarg[ai].act_len) {
            if (ai == 1)
                k->off = 0;
            else if (ai == 2)
                k->sz = arg->uarg[ai].act_len;
            if (k->sz <= sizeof(k->data)) {
                memcpy(k->data, arg->uarg[ai].data, k->sz);
                DLOG("Added KSP: [0x%08X] => 0x%08X @ 0x%08X\n", k->sz, k->id, k->off);
                return 0;
            } else
                ELOG("KSP patch data too large\n");
        } else
            ELOG("Invalid KSP data arg\n");
    } else
        ELOG("Invalid keyslot index type\n");
    my_free(k);
    *pk = NULL;
    return -1;
}

int cmdh_lv0dat(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    struct lv0p_d_s **pd = NULL;
    struct lv0p_d_s *d = cmdh_lv0p_args.d;
    while (d) {
        pd = &d->next;
        d = d->next;
    }
    if (!pd)
        pd = &cmdh_lv0p_args.d;
    *pd = my_malloc(sizeof(struct lv0p_d_s));
    if (!*pd) {
        ELOG("Failed to allocate memory for LV0_DAT command\n");
        return -1;
    }
    d = *pd;
    memset(d, 0, sizeof(struct lv0p_d_s));
    int ai = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        d->dst = arg->uarg[ai].uintgr;
        ai++;
        if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
            d->sz = arg->uarg[ai].uintgr;
            ai++;
            if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
                d->src = arg->uarg[ai].uintgr;
                d->ncopyin = true;
                DLOG("Added LV0_DAT: [0x%08X] 0x%08X => 0x%08X\n", d->sz, d->src, d->dst);
                return 0;
            }
        }
        if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA)) && arg->uarg[ai].act_len) {
            if (ai == 1)
                d->sz = arg->uarg[ai].act_len;
            d->src_va = arg->uarg[ai].data;
            arg->uarg[ai].data = NULL; // transfer ownership to the runner
            DLOG("Added LV0_DAT: [0x%08X] => 0x%08X\n", d->sz, d->dst);
            return 0;
        } else
            ELOG("Invalid LV0_DAT src arg\n");
    } else
        ELOG("Invalid LV0_DAT dest arg\n");
    my_free(d);
    *pd = NULL;
    return -1;
}

int cmdh_lv0x(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    struct lv0p_x_s **px = NULL;
    struct lv0p_x_s *x = cmdh_lv0p_args.x;
    while (x) {
        px = &x->next;
        x = x->next;
    }
    if (!px)
        px = &cmdh_lv0p_args.x;
    *px = my_malloc(sizeof(struct lv0p_x_s));
    if (!*px) {
        ELOG("Failed to allocate memory for LV0X command\n");
        return -1;
    }
    x = *px;
    memset(x, 0, sizeof(struct lv0p_x_s));
    int ai = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        x->arg = arg->uarg[ai].uintgr;
        ai++;
        if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
            x->addr = arg->uarg[ai].uintgr;
            DLOG("Added LV0X: 0x%08X(0x%08X)\n", x->addr, x->arg);
            return 0;
        }
    }
    if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA)) && arg->uarg[ai].act_len) {
        x->c_sz = arg->uarg[ai].act_len;
        x->src_va = arg->uarg[ai].data;
        arg->uarg[ai].data = NULL;
        DLOG("Added LV0X: dyn(0x%08X)\n", x->arg);
        return 0;
    } else
        ELOG("Invalid LV0X src arg\n");
    my_free(x);
    *px = NULL;
    return -1;
}

int cmdh_kblp(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    uint8_t *kblp1 = g_eex_ports.kbl_param;
    uint8_t *kblp2 = (uint8_t *)(*(uint32_t *)(*(uint32_t *)(0x51138a3c) + 0x6c));
    if (!(arg->types & TXTCFG_TYPES_PARSE(0, _UINT))) {
        ELOG("Invalid KBLP offset argument\n");
        return -1;
    }
    int ai = 1;
    uint32_t size = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        size = arg->uarg[ai].uintgr;
        if (!size) {
            ELOG("Invalid KBLP size argument\n");
            return -1;
        }
        ai++;
    }
    if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA, _UINT)) && arg->uarg[ai].act_len) {
        if (!size)
            size = arg->uarg[ai].act_len;
        memcpy(kblp1 + arg->uarg[0].uintgr, cmdh_get_valias(cfg, arg->uarg[ai].uintgr, true), size);
        ILOG("Patched kbl_param at offset 0x%02X with data from 0x%08X\n", arg->uarg[0].uintgr, arg->uarg[ai].data);
        memcpy(kblp2 + arg->uarg[0].uintgr, cmdh_get_valias(cfg, arg->uarg[ai].uintgr, true), size);
        ILOG("Patched kbl_param NP2 at offset 0x%02X with data from 0x%08X\n", arg->uarg[0].uintgr, arg->uarg[ai].data);
        return 0;
    } else
        ELOG("Invalid KBLP src arg\n");
    return -1;
}

int cmdh_armd(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    struct armp_d_s **pd = NULL;
    struct armp_d_s *d = cmdh_armp_args.d;
    while (d) {
        pd = &d->next;
        d = d->next;
    }
    if (!pd)
        pd = &cmdh_armp_args.d;
    *pd = my_malloc(sizeof(struct armp_d_s));
    if (!*pd) {
        ELOG("Failed to allocate memory for ARM D command data\n");
        return -1;
    }
    d = *pd;
    memset(d, 0, sizeof(struct armp_d_s));
    int ai = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        d->dst = cmdh_get_valias(cfg, arg->uarg[ai].uintgr, true);
        ai++;
        if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
            d->sz = arg->uarg[ai].uintgr;
            ai++;
            if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
                d->src = cmdh_get_valias(cfg, arg->uarg[ai].uintgr, true);
                DLOG("Added ARM_DAT: [0x%08X] 0x%08X => 0x%08X\n", d->sz, d->src, d->dst);
                return 0;
            }
        }
        if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA)) && arg->uarg[ai].act_len) {
            if (ai == 1)
                d->sz = arg->uarg[ai].act_len;
            d->src = arg->uarg[ai].data;
            arg->uarg[ai].data = NULL;
            d->src_fa = true; // indicate that the source is a freeable address
            DLOG("Added ARM_DAT: [0x%08X] => 0x%08X\n", d->sz, d->dst);
            return 0;
        } else
            ELOG("Invalid ARM_DAT src arg\n");
    } else
        ELOG("Invalid ARM_DAT dest arg\n");
    my_free(d);
    *pd = NULL;
    return -1;
}

int cmdh_armx(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    struct armp_x_s **px = NULL;
    struct armp_x_s *x = cmdh_armp_args.x;
    while (x) {
        px = &x->next;
        x = x->next;
    }
    if (!px)
        px = &cmdh_armp_args.x;
    *px = my_malloc(sizeof(struct armp_x_s));
    if (!*px) {
        ELOG("Failed to allocate memory for ARM X command data\n");
        return -1;
    }
    x = *px;
    memset(x, 0, sizeof(struct armp_x_s));
    if (arg->types & TXTCFG_TYPES_PARSE(2, _UINT))
        x->arg2 = cmdh_get_valias(cfg, arg->uarg[2].uintgr, true);
    
    int ai = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
        x->arg = arg->uarg[ai].uintgr;
        ai++;
        if (arg->types & TXTCFG_TYPES_PARSE(ai, _UINT)) {
            x->src = cmdh_get_valias(cfg, arg->uarg[ai].uintgr, true);
            DLOG("Added ARM X: 0x%08X(0x%08X)\n", x->src, x->arg);
            return 0;
        }
    }
    if ((arg->types & TXTCFG_TYPES_PARSE(ai, _RDATA, _FDATA)) && arg->uarg[ai].act_len) {
        x->c_sz = arg->uarg[ai].act_len;
        x->src = arg->uarg[ai].data;
        arg->uarg[ai].data = NULL;
        DLOG("Added ARM X: dyn(0x%08X)\n", x->arg);
        return 0;
    } else
        ELOG("Invalid ARM X src arg\n");
    my_free(x);
    *px = NULL;
    return -1;
}

int cmdh_mntinit(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    int mntidx = -1;
    int p_m = -1, p_x = -1, p_a = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(0, _UINT)) {
        if (arg->uarg[0].uintgr >= STOR_MAX_MOUNTS) {
            ELOG("Invalid mount idx %d\n", arg->uarg[0].uintgr);
            return -1;
        }
        mntidx = arg->uarg[0].uintgr;
    } else if (arg->types & TXTCFG_TYPES_PARSE(0, _ASCII)) {
        mntidx = -1;
        for (int i = 0; i < STOR_MAX_MOUNTS; i++) {
            if (!my_strncmp(arg->uarg[0].ascii, fmgr_mount_names[i], 3)) {
                mntidx = i;
                break;
            }
        }
        if (mntidx < 0) {
            ELOG("Invalid mount name %s\n", arg->uarg[0].ascii);
            return -1;
        }
    } else {
        ELOG("No mount index or name specified\n");
        return -1;
    }
    if (arg->types & TXTCFG_TYPES_PARSE(1, _UINT)) {
        if (arg->uarg[1].uintgr >= MOUNT_MASTER_COUNT) {
            ELOG("Invalid mount master idx %d\n", arg->uarg[1].uintgr);
            return -1;
        }
        p_m = arg->uarg[1].uintgr;
    } else if (arg->types & TXTCFG_TYPES_PARSE(1, _ASCII)) {
        if (!my_strncmp(arg->uarg[1].ascii, "eMMC", 4) || !my_strncmp(arg->uarg[1].ascii, "emmc", 4) || !my_strncmp(arg->uarg[1].ascii, "EMMC", 4))
            p_m = MOUNT_MASTER_EMMC;
        else if (!my_strncmp(arg->uarg[1].ascii, "GCSD", 4) || !my_strncmp(arg->uarg[1].ascii, "gcsd", 4) || !my_strncmp(arg->uarg[1].ascii, "sd", 2) || !my_strncmp(arg->uarg[1].ascii, "SD", 2))
            p_m = MOUNT_MASTER_GCSD;
        else {
            ELOG("Invalid mount master name %s\n", arg->uarg[1].ascii);
            return -1;
        }
    } else {
        DLOG("No mount master provided, using default os0\n");
        uint32_t os0_nfo = fmgr_get_nskbl_os0(false);
        p_m = FMGR_MASTER_SCAN_UNPACK(MASTER, os0_nfo);
        p_x = FMGR_MASTER_SCAN_UNPACK(PARTITION, os0_nfo);
        p_a = FMGR_MASTER_SCAN_UNPACK(ACTIVE, os0_nfo);
    }
    if (arg->types & TXTCFG_TYPES_PARSE(2, _UINT)) {
        if (arg->uarg[2].uintgr >= STOR_PART_COUNT) {
            ELOG("Invalid stor partition idx %d\n", arg->uarg[2].uintgr);
            return -1;
        }
        p_x = arg->uarg[2].uintgr;
    } else if (arg->types & TXTCFG_TYPES_PARSE(2, _ASCII)) {
        for (int i = 0; i < STOR_PART_COUNT; i++) {
            if (!my_strncmp(arg->uarg[2].ascii, get_partition_name(i), arg->uarg[2].act_len)) {
                p_x = i;
                break;
            }
        }
        if (p_x < 0) {
            ELOG("Invalid stor partition name %s\n", arg->uarg[2].ascii);
            return -1;
        }
    } else if (p_x < 0) {
        ELOG("No stor partition provided!\n");
        return -1;
    }
    if (arg->types & TXTCFG_TYPES_PARSE(3, _UINT)) {
        if (arg->uarg[3].uintgr >= 2) {
            ELOG("Invalid stor partition active %d\n", arg->uarg[3].uintgr);
            return -1;
        }
        p_a = arg->uarg[3].uintgr;
    } else if (arg->types & TXTCFG_TYPES_PARSE(3, _ASCII)) {
        if (!my_strncmp(arg->uarg[3].ascii, "act", 3)) {
            p_a = 1;
        } else if (!my_strncmp(arg->uarg[3].ascii, "ina", 3)) {
            p_a = 0;
        } else {
            ELOG("Invalid stor partition active %s\n", arg->uarg[3].ascii);
            return -1;
        }
    }
    ILOG("Initializing storage master %d\n", p_m);
    if (stor_init_master(p_m) < 0) {
        ELOG("Failed to initialize storage master %d\n", p_m);
        return -1;
    }
    ILOG("Unmounting FF & STOR mounts @ idx %d\n", mntidx);
    fmgr_mount(false, mntidx);
    stor_umount(mntidx);
    DLOG("Mounting storage master %d partition %d active %d at mount idx %d\n", p_m, p_x, p_a, mntidx);
    if (stor_init_mount(mntidx, p_m, p_x, p_a) < 0) {
        ELOG("Failed to mount storage master %d partition %d active %d at mount idx %d\n", p_m, p_x, p_a, mntidx);
        return -1;
    }
    return fmgr_mount(true, mntidx);
}

int cmdh_lv0init(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
	if (lv0_initialized) {
		WLOG("WARN: LV0 is already initialized, skipping lv0_init\n");
		return 0;
	}
    if (arg->types & TXTCFG_TYPES_PARSE(0, _ASCII)) {
        if (fmgr_get_file_size(arg->uarg[0].ascii)) {
            DLOG("Initializing SPL with update_sm @ %s\n", arg->uarg[0].ascii);
            return lv0_init(arg->uarg[0].ascii);
        } else {
            ELOG("Invalid update_sm path %s\n", arg->uarg[0].ascii);
            return -1;
        }
    }
    ELOG("No update_sm path provided\n");
    return -1;
}

int cmdh_memgr(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    uint32_t m_alias = 0xF0000000;
    if (arg->types & TXTCFG_TYPES_PARSE(0, _UINT))
        m_alias = arg->uarg[0].uintgr;
    else {
        ELOG("No memory alias provided\n");
        return -1;
    }

    if (idx == CMDH_M_FREE) {
        void *va = cmdh_get_valias(cfg, m_alias, false);
        if (!va) {
            ELOG("Invalid memory alias 0x%08X for free\n", m_alias);
            return -1;
        }
        DLOG("Freeing memory alias 0x%08X at VA 0x%08X\n", m_alias, va);
        return my_free(va);
    }

    uint32_t m_type = MEMBLOCK_TYPE_RW;
    if (arg->types & TXTCFG_TYPES_PARSE(1, _UINT))
        m_type = arg->uarg[1].uintgr;
    else if (arg->types & TXTCFG_TYPES_PARSE(1, _ASCII)) {
        if (!my_strncmp(arg->uarg[1].ascii, "rx", 2))
            m_type = MEMBLOCK_TYPE_RX;
        else if (!my_strncmp(arg->uarg[1].ascii, "rw", 2))
            m_type = MEMBLOCK_TYPE_RW;
        else if (!my_strncmp(arg->uarg[1].ascii, "uc", 2))
            m_type = MEMBLOCK_TYPE_UCRW;
        else {
            ELOG("Invalid memblock type %s\n", arg->uarg[1].ascii);
            return -1;
        }
    } else {
        ELOG("No memblock type provided\n");
        return -1;
    }

    if (idx == CMDH_M_RMAP) {
        void *va = cmdh_get_valias(cfg, m_alias, false);
        if (!va) {
            ELOG("Invalid memory alias 0x%08X for rmap\n", m_alias);
            return -1;
        }
        DLOG("Remapping memory alias 0x%08X at VA 0x%08X with type 0x%08X\n", m_alias, va, m_type);
        return rmemblock_remap(va, m_type);
    }

    uint32_t m_size = 0;
    const char *m_file = NULL;
    if (arg->types & TXTCFG_TYPES_PARSE(2, _UINT)) {
        m_size = arg->uarg[2].uintgr;
        if (!m_size || (m_size > RMEMBLOCK_MAX_SIZE)) {
            ELOG("Invalid memory size 0x%08X\n", m_size);
            return -1;
        }
    } else if (arg->types & TXTCFG_TYPES_PARSE(2, _ASCII)) {
        if (my_strncmp(arg->uarg[2].ascii, "mnt", 3) && my_strncmp(arg->uarg[2].ascii, "os0", 3)) {
            ELOG("Invalid file source for memblock %s\n", arg->uarg[2].ascii);
            return -1;
        }
        m_file = arg->uarg[2].ascii;
        m_size = fmgr_get_file_size(m_file);
        if (!m_size || (m_size > RMEMBLOCK_MAX_SIZE)) {
            ELOG("Invalid memory size (0<)0x%08X(<=0x%08X) for file %s\n", m_size, RMEMBLOCK_MAX_SIZE, m_file);
            return -1;
        }
    } else {
        ELOG("No memory size provided\n");
        return -1;
    }

    uint32_t m_paddr = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(3, _UINT))
        m_paddr = arg->uarg[3].uintgr;

    DLOG("Allocating memory alias 0x%08X with size 0x%08X, type %d, paddr 0x%08X\n", m_alias, m_size, m_type, m_paddr);
    void *va = rmemblock_alloc(m_size, m_type, m_paddr);
    if (!va) {
        ELOG("Failed to allocate memory alias 0x%08X\n", m_alias);
        return -1;
    }

    if (m_file) {
        uint32_t bytes_read = 0;
        ILOG("Loading file %s into memory alias 0x%08X at VA 0x%08X\n", m_file, m_alias, va);
        if (!fmgr_get_file(m_file, va, m_size, 0, &bytes_read) || (bytes_read != m_size)) {
            ELOG("Failed to load file %s into memory alias 0x%08X (%d bytes read)\n", m_file, m_alias, bytes_read);
            rmemblock_free(va);
            return -1;
        }
        DLOG("Loaded file %s into memory alias 0x%08X (%d bytes)\n", m_file, m_alias, bytes_read);
    }

    for (int i = 0; i < TXTCFG_MAX_OVERLAYS; i++) {
        if (cfg->overlay[i].alias == m_alias) {
            DLOG("WARN: Memory alias 0x%08X was already assigned to overlay %d, overriding\n", m_alias, i);
            cfg->overlay[i].size = 0; // clear existing overlay to reuse the slot
        }
        if (!cfg->overlay[i].size) {
            cfg->overlay[i].alias = m_alias;
            cfg->overlay[i].va = va;
            cfg->overlay[i].size = m_size;
            ILOG("Assigned memory alias 0x%08X to overlay %d (VA 0x%08X - 0x%08X)\n", m_alias, i, va, (uint32_t)va + m_size);
            return 0;
        }
    }
    ELOG("No available overlay slots for memory alias 0x%08X\n", m_alias);
    return -1;
}

int cmdh_ppatch(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    
    bool do_cleanup = true;
    if (arg->types & TXTCFG_TYPES_PARSE(1, _UINT))
        do_cleanup = arg->uarg[1].uintgr ? true : false;
    else if (arg->types & TXTCFG_TYPES_PARSE(1, _ASCII)) {
        if (!my_strncmp(arg->uarg[1].ascii, "yes", 3))
            do_cleanup = true;
        else if (!my_strncmp(arg->uarg[1].ascii, "no", 2))
            do_cleanup = false;
        else {
            ELOG("Invalid patch cleanup argument %s\n", arg->uarg[1].ascii);
            return -1;
        }
    }

    int iret = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(0, _ASCII)) {
        if (!my_strncmp(arg->uarg[0].ascii, "arm", 3))
            iret = cmdh_apply_pchains(NULL, &cmdh_armp_args, do_cleanup);
        else if (!my_strncmp(arg->uarg[0].ascii, "lv0", 3))
            iret = cmdh_apply_pchains(&cmdh_lv0p_args, NULL, do_cleanup);
        else if (!my_strncmp(arg->uarg[0].ascii, "all", 3))
            iret = cmdh_apply_pchains(&cmdh_lv0p_args, &cmdh_armp_args, do_cleanup);
        else {
            ELOG("Invalid patch chain %s\n", arg->uarg[0].ascii);
            return -1;
        }
    } else {
        ELOG("No patch chain provided\n");
        return -1;
    }

    DLOG("Applied patch chain(s) with result 0x%08X\n", iret);
    return iret;
}

int cmdh_filerw(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg) {
    if (!arg || !arg->handler) {
        ELOG("Invalid arguments for command %s\n", cmdh_args[idx].name);
        return -1;
    }
    uint32_t rw_size = 0;
    if (arg->types & TXTCFG_TYPES_PARSE(2, _UINT))
        rw_size = arg->uarg[2].uintgr;

    if ((arg->types & TXTCFG_TYPES_PARSE(0, _ASCII)) && (arg->types & TXTCFG_TYPES_PARSE(1, _ASCII))) // file->file
        return fmgr_copy_file_ws(arg->uarg[1].ascii, arg->uarg[0].ascii, rw_size);
    else if ((arg->types & TXTCFG_TYPES_PARSE(0, _UINT)) && (arg->types & TXTCFG_TYPES_PARSE(1, _ASCII))) // file->mem
        return fmgr_get_file(arg->uarg[1].ascii, cmdh_get_valias(cfg, arg->uarg[0].uintgr, true), rw_size, 0, NULL) ? 0 : -1;
    else if ((arg->types & TXTCFG_TYPES_PARSE(0, _ASCII)) && (arg->types & TXTCFG_TYPES_PARSE(1, _UINT))) { // mem->file
        if (!rw_size) {
            ELOG("Invalid size for mem->file copy\n");
            return -1;
        }
        return fmgr_set_file(arg->uarg[0].ascii, cmdh_get_valias(cfg, arg->uarg[1].uintgr, true), rw_size, NULL);
    }

    ELOG("Invalid FILERW command arguments\n");
    return -1;
}

const void *cmdh_dispatch_table(int idx) {
    void *dispatch_table[CMDH_DCOUNT] = {
        NULL,
        cmdh_allow_livexe,
        cmdh_breakproxy,
        cmdh_mntinit,
        cmdh_lv0init,
        cmdh_lv0stck,
        cmdh_ks,
        cmdh_lv0dat,
        cmdh_lv0x,
        cmdh_armd,
        cmdh_armx,
        cmdh_kblp,
        cmdh_memgr,
        cmdh_memgr,
        cmdh_memgr,
        cmdh_ppatch,
        cmdh_filerw
    };
    if (idx)
        return dispatch_table[idx];
    for (int i = 0; i < CMDH_DCOUNT; i++)
        cmdh_args[i].handler = dispatch_table[i];
    return NULL;
}

static char *find_endline(char *start, char *end) {
    for (char *ret = start; ret < end; ret++) {
        if (*(uint16_t *)ret == 0x0A0D || *(uint8_t *)ret == 0x0A)
            return ret;
    }
    return end;
}

static char *find_nextline(char *current_line_end, char *end) {
    for (char *next_line = current_line_end; next_line < end; next_line++) {
        if (*(uint8_t *)next_line != 0x0D && *(uint8_t *)next_line != 0x0A && *(uint8_t *)next_line != 0x00)
            return next_line;
    }
    return NULL;
}

static int txtcfg_getCmdIDX(struct txtcfg_s *cfg, char *line, char *end) {
    int cmd_len = 0;
    int max_cmd_len = end - line;
    for (int i = 1; i < cfg->arg_count; i++) {
        cmd_len = strlen(cfg->arg[i].name);
        if (cmd_len < max_cmd_len) {
            if (!memcmp(line, cfg->arg[i].name, cmd_len))
                return i;
        }
    }
    return 0;
}

static void txtcfg_prepCmdByIDX(struct txtcfg_s *cfg, int idx, char *arg, char *end) {
    int ret = -1;
    ILOG("Preparing command by index: %d\n", idx);
    struct txtcfg_arg_s *pcfg = &cfg->arg[idx];
    if (pcfg->parsed) {
        WLOG("WARN: Command already prepared\n");
        goto txtcfg_brexit;
    }
    arg += strlen(cfg->arg[idx].name);
    if (*(uint8_t *)arg != 0x3D) {
        ELOG("Invalid command format (!=)\n");
        goto txtcfg_brexit;
    }
    arg++;

    // cut invalid and comments (" " and "#")
    for (int i = 0; i < (int)(end - arg); i++) {
        if (*(uint8_t *)(arg + i) == 0x20 || *(uint8_t *)(arg + i) == 0x23) {
            end = arg + i;
            break;
        }
    }

    char *carg = (char *)my_malloc((uint32_t)(end - arg) + 1);
    if (!carg) {
        ELOG("Failed to allocate memory for command argument\n");
        goto txtcfg_brexit;
    }
    char *cend = carg + (end - arg);
    memset(carg, 0, cend - carg);
    memcpy(carg, arg, end - arg);

    char *sub_arg = carg;
    for (int a = 0; a < 4; a++) {
        char *sub_end = my_strnchr(sub_arg, ',', cend - sub_arg);
        if (!sub_end || sub_end > cend)
            sub_end = cend;
        *sub_end = 0;
        pcfg->uarg[a].uintgr = 0;
        pcfg->uarg[a].act_len = 0;
        pcfg->types &= ~TXTCFG_TYPES_PARSE_ALL(a);
        if ((pcfg->types & TXTCFG_TYPES_ALLOW(a, _UINT)) && !my_strncmp(sub_arg, "0x", 2)) {
            sub_arg += 2;
            pcfg->uarg[a].act_len = strlen(sub_arg) / 2;
            if ((pcfg->uarg[a].act_len < pcfg->uarg[a].min_len) || (pcfg->uarg[a].act_len > pcfg->uarg[a].max_len)) {
                ELOG("Command argument %d length out of bounds: 0x%08X<=0x%08X=>0x%08X [UINT]\n", a, pcfg->uarg[a].min_len, pcfg->uarg[a].act_len,
                    pcfg->uarg[a].max_len);
                goto txtcfg_fbrexit;  // hard break
            }
            if (antoh(sub_arg, (uint8_t *)&pcfg->uarg[a].uintgr, pcfg->uarg[a].act_len) < 0) {
                ELOG("Could not convert command argument %d [UINT]\n", a);
                goto txtcfg_fbrexit;
            }
            if (pcfg->uarg[a].act_len == 4)
                pcfg->uarg[a].uintgr = BSWAP32(pcfg->uarg[a].uintgr);
            else if (pcfg->uarg[a].act_len == 3)
                pcfg->uarg[a].uintgr = BSWAP24(pcfg->uarg[a].uintgr);
            else if (pcfg->uarg[a].act_len == 2)
                pcfg->uarg[a].uintgr = BSWAP16(pcfg->uarg[a].uintgr);
            pcfg->types |= TXTCFG_TYPES_PARSE(a, _UINT);
        } else if ((pcfg->types & TXTCFG_TYPES_ALLOW(a, _FDATA)) && (!my_strncmp(sub_arg, "mnt", 3) || !my_strncmp(sub_arg, "os0", 3))) {
            pcfg->uarg[a].act_len = fmgr_get_file_size(sub_arg);
            if ((pcfg->uarg[a].act_len < pcfg->uarg[a].min_len) || (pcfg->uarg[a].act_len > pcfg->uarg[a].max_len)) {
                ELOG("Command argument %d length out of bounds: 0x%08X<=0x%08X=>0x%08X [FDATA]\n", a, pcfg->uarg[a].min_len, pcfg->uarg[a].act_len,
                    pcfg->uarg[a].max_len);
                goto txtcfg_fbrexit;
            }
            pcfg->uarg[a].data = fmgr_get_file(sub_arg, NULL, pcfg->uarg[a].act_len, 0, NULL);
            if (!pcfg->uarg[a].data) {
                ELOG("Could not read file for command argument %d [FDATA]\n", a);
                goto txtcfg_fbrexit;
            }
            pcfg->types |= TXTCFG_TYPES_PARSE(a, _FDATA);
        } else if (pcfg->types & TXTCFG_TYPES_ALLOW(a, _RDATA)) {
            pcfg->uarg[a].act_len = strlen(sub_arg) / 2;
            if ((pcfg->uarg[a].act_len < pcfg->uarg[a].min_len) || (pcfg->uarg[a].act_len > pcfg->uarg[a].max_len)) {
                ELOG("Command argument %d length out of bounds: 0x%08X<=0x%08X=>0x%08X [RDATA]\n", a, pcfg->uarg[a].min_len, pcfg->uarg[a].act_len,
                    pcfg->uarg[a].max_len);
                goto txtcfg_fbrexit;  // hard break
            }
            pcfg->uarg[a].data = my_malloc(pcfg->uarg[a].act_len);
            if (!pcfg->uarg[a].data) {
                ELOG("Could not allocate memory for command argument %d [RDATA]\n", a);
                goto txtcfg_fbrexit;
            }
            if (antoh(sub_arg, pcfg->uarg[a].data, pcfg->uarg[a].act_len) < 0) {
                ELOG("Could not convert command argument %d [RDATA]\n", a);
                my_free(pcfg->uarg[a].data);
                pcfg->uarg[a].data = NULL;
                goto txtcfg_fbrexit;
            }
            pcfg->types |= TXTCFG_TYPES_PARSE(a, _RDATA);
        } else if (pcfg->types & TXTCFG_TYPES_ALLOW(a, _ASCII)) {
            pcfg->uarg[a].act_len = strlen(sub_arg);
            if ((pcfg->uarg[a].act_len < pcfg->uarg[a].min_len) || (pcfg->uarg[a].act_len > pcfg->uarg[a].max_len)) {
                ELOG("Command argument %d length out of bounds: 0x%08X<=0x%08X=>0x%08X [ASCII]\n", a, pcfg->uarg[a].min_len, pcfg->uarg[a].act_len,
                    pcfg->uarg[a].max_len);
                goto txtcfg_fbrexit;  // hard break
            }
            pcfg->uarg[a].ascii = my_malloc(pcfg->uarg[a].act_len + 1);
            if (!pcfg->uarg[a].ascii) {
                ELOG("Could not allocate memory for command argument %d [ASCII]\n", a);
                goto txtcfg_fbrexit;
            }
            memcpy(pcfg->uarg[a].ascii, sub_arg, pcfg->uarg[a].act_len);
            pcfg->uarg[a].ascii[pcfg->uarg[a].act_len] = 0;
            pcfg->types |= TXTCFG_TYPES_PARSE(a, _ASCII);
        }
        if (sub_end == cend)
            break;
        sub_arg = sub_end + 1;
    }

    ret = 0;

    if (pcfg->exec && pcfg->handler) {
        ILOG("Executing handler for command %d\n", idx);
        pcfg->handler(idx, pcfg, cfg);
        for (int a = 0; a < 4; a++) {
            if ((pcfg->types & TXTCFG_TYPES_PARSE(a, _FDATA)) || (pcfg->types & TXTCFG_TYPES_PARSE(a, _RDATA)) ||
                (pcfg->types & TXTCFG_TYPES_PARSE(a, _ASCII))) {
                if (pcfg->uarg[a].data) {
                    DLOG("Freeing command argument %d data\n", a);
                    my_free(pcfg->uarg[a].data);
                }
            }
            pcfg->uarg[a].act_len = 0;
            pcfg->uarg[a].uintgr = 0;
            pcfg->types &= ~TXTCFG_TYPES_PARSE_ALL(a);
        }
    } else {
        pcfg->parsed = true;
	}

txtcfg_fbrexit:
    my_free(carg);
txtcfg_brexit:
	if (ret < 0 && cfg->brhandler)
		cfg->brhandler(idx, NULL, cfg);
    return;
}

void txtcfg_parse(struct txtcfg_s *cfg) {
    char *startconfig = cfg->buf.va;
    char *endconfig = startconfig + cfg->buf.size;

    char *current_line = startconfig;
    char *end_line = startconfig;
    int command_idx = 0;
    while (current_line < endconfig) {
        end_line = find_endline(current_line, endconfig);
        command_idx = txtcfg_getCmdIDX(cfg, current_line, end_line);
        if (command_idx) {
            txtcfg_prepCmdByIDX(cfg, command_idx, current_line, end_line);
            if (cfg->dead)
                break;
        }
        current_line = find_nextline(end_line, endconfig);
        if (!current_line)
            break;
    }
}

void txtcfg_cleanup(struct txtcfg_s *cfg) {
    for (int i = 0; i < cfg->arg_count; i++) {
        struct txtcfg_arg_s *arg = &cfg->arg[i];
        if (arg->parsed) {
            for (int a = 0; a < 4; a++) {
                if ((arg->types & TXTCFG_TYPES_PARSE(a, _FDATA)) || (arg->types & TXTCFG_TYPES_PARSE(a, _RDATA)) ||
                    (arg->types & TXTCFG_TYPES_PARSE(a, _ASCII))) {
                    if (arg->uarg[a].data) {
                        DLOG("Freeing command %d argument %d data\n", i, a);
                        my_free(arg->uarg[a].data);
                    }
                }
                arg->uarg[a].uintgr = 0;
                arg->uarg[a].act_len = 0;
                arg->types &= ~TXTCFG_TYPES_PARSE_ALL(a);
            }
            arg->parsed = false;
        }
    }
}

int txtcfg_loadExec(struct txtcfg_s *cfg, bool cleanup) {
    if (!cfg->buf.va) {
        ILOG("Loading config file: %s\n", cfg->fpath);
        cfg->buf.va = fmgr_get_file(cfg->fpath, NULL, 0, 0, &cfg->buf.size);
        if (!cfg->buf.va) {
            ELOG("Could not load config file: %s\n", cfg->fpath);
            return -1;
        }
    } else
        DLOG("Using preloaded config file: %s\n", cfg->fpath);
    txtcfg_parse(cfg);
    if (!cfg->dead) {
        ILOG("Executing config file: %s\n", cfg->fpath);
        for (int i = 0; i < cfg->arg_count; i++) {
            if (cfg->arg[i].parsed && cfg->arg[i].handler)
                cfg->arg[i].handler(i, &cfg->arg[i], cfg);
        }
    }
    if (cleanup) {
        DLOG("Freeing config file: %s\n", cfg->fpath);
        txtcfg_cleanup(cfg);
        my_free(cfg->buf.va);
        cfg->buf.va = NULL;
        cfg->buf.size = 0;
    }
    if (cfg->dead) {
        ELOG("loadExec dead\n");
        return -2;
    }
    return 0;
}

int txtcfg_lxPath(char *path, bool cleanup, struct txtcfg_s *cfg) {
	ILOG("txtcfg_lxPath(path=%s, cleanup=%d, cfg=0x%X)\n", path, cleanup, cfg);
	if (!cfg) {
		ELOG("No configuration structure provided!\n");
		return -1;
	}
	if (path) {
    	if (cfg->fpath) {
        	ELOG("Config structure in use by: %s\n", cfg->fpath);
        	return -1;
    	}
		cfg->fpath = path;
	} else if (!cfg->fpath) {
		ELOG("No configuration file path provided\n");
		return -1;
	}
	if (cfg->buf.va || cfg->buf.size) {
		ELOG("Config structure already loaded: 0x%08X [0x%08X]\n", cfg->buf.va, cfg->buf.size);
		return -1;
	}
	if (!cfg->arg_count || !cfg->arg || !cfg->h_dispatcher) {
		ELOG("Invalid configuration arguments provided: %d %d %d\n", cfg->arg_count, cfg->arg != NULL, cfg->h_dispatcher != NULL);
		return -1;
	}
	int ret = (int)cfg->h_dispatcher(0);
	if (ret) {
		ELOG("Command handler dispatch failed: 0x%08X\n", ret);
		return ret;
	}
	ret = txtcfg_loadExec(cfg, cleanup);
    ILOG("txtcfg_loadExec ret: 0x%08X\n", ret);
	if (cleanup) {
		if (path)
			cfg->fpath = NULL;
		cfg->brhandler = NULL;
        cfg->dead = false;
	}
    return ret;
}

int cmdh_apply_pchains(struct lv0p_arg_s *lv0c, struct armp_arg_s *armc, bool cleanup) {
	ILOG("cmdh_apply_pchains(lv0c=0x%08X, armp=0x%08X)\n", lv0c, armc);
	int lv0ret = 0;
	int armret = 0;
    if (armc && (armc->d || armc->x)) {
        ILOG("Starting armp_run with args: d=0x%08X, x=0x%08X\n", (unsigned int)armc->d, (unsigned int)armc->x);
        int ret = armp_run(armc, CHAIN_FREE_NESTED);
        DLOG("armp_run returned: 0x%08X\n", ret);
        if (cleanup) {
            void *frbuf = NULL;
            struct armp_d_s *d = armc->d;
            while (d) {
                ILOG("ARMP D(0x%08X @ 0x%08X => 0x%08X) ret 0x%08X\n", d->sz, d->src, d->dst, d->ret);
                frbuf = (void *)d;
                d = d->next;
                my_free(frbuf);
            }
            struct armp_x_s *x = armc->x;
            while (x) {
                ILOG("ARMP X(0x%08X(0x%08X)) ret 0x%08X\n", x->src, x->arg, x->ret);
                frbuf = (void *)x;
                x = x->next;
                my_free(frbuf);
            }
			armc->d = NULL;
			armc->x = NULL;
        }
    }
    if (lv0c && (lv0c->k || lv0c->d || lv0c->x)) {
        ILOG("Starting BIG lv0p_run with args: k=0x%08X, d=0x%08X, x=0x%08X\n", (unsigned int)lv0c->k, (unsigned int)lv0c->d, (unsigned int)lv0c->x);
        lv0ret = lv0p_run(lv0c, LV0_SPL_LV0P_BIG_ADDR, LV0_SPL_LV0P_BIG_SIZE, CHAIN_FREE_NESTED);
        DLOG("lv0p_run returned: 0x%08X\n", lv0ret);
        if (cleanup) {
            void *frbuf = NULL;
            struct lv0p_k_s *k = lv0c->k;
            while (k) {
                ILOG("LVOP K(0x%08X @ 0x%08X <= 0x%08X) ret 0x%08X\n", k->off, k->id, k->sz, k->ret);
                frbuf = (void *)k;
                k = k->next;
                my_free(frbuf);
            }
            struct lv0p_d_s *d = lv0c->d;
            while (d) {
                ILOG("LVOP D(0x%08X @ 0x%08X => 0x%08X) ret 0x%08X\n", d->sz, d->src, d->dst, d->ret);
                frbuf = (void *)d;
                d = d->next;
                my_free(frbuf);
            }
            struct lv0p_x_s *x = lv0c->x;
            while (x) {
                ILOG("LVOP X(0x%08X(0x%08X)) ret 0x%08X\n", x->addr, x->arg, x->ret);
                frbuf = (void *)x;
                x = x->next;
                my_free(frbuf);
            }
			lv0c->me = NULL;
            lv0c->stack = 0;
			lv0c->w = NULL;
			lv0c->k = NULL;
			lv0c->d = NULL;
			lv0c->x = NULL;
        }
    }
    return lv0ret | armret;
}

static struct txtcfg_s cmdh_default_cfg = {
    .fpath = NULL,
    .buf = {.va = NULL, .size = 0},
    .arg_count = CMDH_DCOUNT,
    .arg_hardc = CMDH_USTART,
    .arg = cmdh_args,
    .h_dispatcher = cmdh_dispatch_table,
    .brhandler = NULL,
    .overlay = {{0}, {0}, {0}, {0}},
    .dead = false
};

int cmdh_lxp(char *fpath) {
	ILOG("cmdh_lxp(fpath=%s)", fpath);
	if (!fpath) {
		ELOG("No file path provided for LV0P command\n");
		return -1;
	}
    if (cmdh_lv0p_args.k || cmdh_lv0p_args.d || cmdh_lv0p_args.x || cmdh_lv0p_args.w) {
        ELOG("Default LV0 patch chain already in use\n");
		return -1;
    }
    if (cmdh_armp_args.d || cmdh_armp_args.x) {
        ELOG("Default ARM patch chain already in use\n");
		return -1;
    }
    int ret = txtcfg_lxPath(fpath, true, &cmdh_default_cfg);
    DLOG("txtcfg_lxPath returned: 0x%08X\n", ret);
	if (ret >= 0) {
		ret = cmdh_apply_pchains(&cmdh_lv0p_args, &cmdh_armp_args, true);
		DLOG("cmdh_apply_pchains returned: 0x%08X\n", ret);
	}
    return ret;
}