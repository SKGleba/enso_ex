#include "lv0.h"

#include "fmgr.h"
#include "utils.h"

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
static const unsigned char inject_framework_nmp[] = {
  0xa0, 0x6f, 0x16, 0x4d, 0x12, 0x4e, 0x1a, 0x7b, 0x0e, 0x4b, 0x00, 0x53,
  0x06, 0x43, 0x16, 0xd3, 0x10, 0x80, 0xfa, 0x03, 0xfe, 0x00, 0x01, 0xc3,
  0x00, 0x01, 0x21, 0xc2, 0x85, 0x1f, 0x24, 0xc2, 0x00, 0x02, 0x00, 0xd1,
  0x9e, 0x80, 0x0f, 0x10, 0x06, 0x40, 0x72, 0xd3, 0x03, 0x80, 0x01, 0xc2,
  0x79, 0xdc, 0x39, 0x02, 0x74, 0xd3, 0x03, 0x80, 0x01, 0xc2, 0x9a, 0x00,
  0x39, 0x02, 0xfe, 0xd3, 0x0a, 0x80, 0x01, 0xc2, 0x10, 0x06, 0x39, 0x02,
  0x07, 0x43, 0x30, 0x00, 0x13, 0x4e, 0x17, 0x4d, 0x0f, 0x4b, 0x60, 0x6f,
  0xbe, 0x10, 0x00, 0x00, 0xf0, 0xff, 0x1f, 0x00
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
	LOG("lv0_load_sm(path=%s)\n", path);
	if (lv0_sm_loaded) {
		LOG("Error: SM already loaded!\n");
		return -1;
	}
	uint32_t sm_size = fmgr_get_file_size(path);
	if (!sm_size) {
		LOG("Error: SM file size is 0!\n");
		return -1;
	}
    void *sm_buf = rmemblock_alloc((sm_size + 0xfff) & 0xfffff000, 0x10208006, 0);  // fmgr_get_file(path, NULL, 0, 0, &sm_size);
    if (!fmgr_get_file(path, sm_buf, sm_size, 0, NULL)) {
        LOG("Failed to cache the SM from %s\n", path);
		return -1;
    }
    void *cmd_buf = rmemblock_alloc(0x1000, 0x10208006, 0);
    if (!cmd_buf) {
		LOG("Failed to allocate command buffer for SM\n");
		my_free(sm_buf);
		return -1;
	}
    struct lv0_ctx130_s ctx130;
	memset(&ctx130, 0, sizeof(struct lv0_ctx130_s));
	memcpy(ctx130.data0, lv0_ctx_130_data, sizeof(lv0_ctx_130_data));
	ctx130.pathId = 2; // os0
	ctx130.self_type = (ctx130.self_type & 0xFFFFFFF0) | 2; // set self_type to user mode
    struct lv0_paddr_list_s paddr_list;
    struct lv0_pa_pair_s pairs[8];
	struct lv0_pa_pair_s vrange;
	vrange.addr = (uint32_t)sm_buf;
	vrange.length = sm_size;
	paddr_list.size = sizeof(paddr_list);
	paddr_list.list = &pairs[0];
	paddr_list.list_size = 8;
	LOG("Get pAddr list for SM: addr=0x%08X, length=0x%08X\n", vrange.addr, vrange.length);
	int ret = nskbl_get_paddr_list(&vrange, &paddr_list);
	if (ret < 0 || paddr_list.ret_count == 0) {
		LOG("Failed to get PAddr list for SM : 0x%08X\n", ret);
		goto lv0_lsm_end;
	}
	uint32_t paddr_pairs_paddr = 0;
	memcpy(cmd_buf, &pairs, paddr_list.ret_count * sizeof(struct lv0_pa_pair_s));
	LOG("Get PAddr for command buffer: addr=0x%08X, length=0x%08X\n", (uint32_t)cmd_buf, sizeof(struct lv0_pa_pair_s) * paddr_list.ret_count);
	ret = nskbl_get_paddr_single(cmd_buf, &paddr_pairs_paddr);
	if (ret < 0 || !paddr_pairs_paddr) {
		LOG("Failed to get PAddr for command buffer: 0x%08X\n", ret);
		goto lv0_lsm_end;
	}
	LOG("Invoke SM with PAddr list: paddr=0x%08X, count=%d\n", paddr_pairs_paddr, paddr_list.ret_count);
	ret = nskbl_smtool_invoke(0, paddr_pairs_paddr, paddr_list.ret_count, NULL, &ctx130, (int *)NSKBL_SMTOOL_CTX);
	if (ret < 0) {
		LOG("Failed to invoke SM for TZ: 0x%08X\n", ret);
		goto lv0_lsm_end;
	}
    v16p 0x51016cf2 = 0x2000; // remove the loadsm call from load_kprauthsm (since ussm is already loaded)
    v16p 0x51016cf4 = 0x2000; // ^
	nskbl_clean_dcache((void *)0x51016cf0, 0x20);
	nskbl_flush_icache();
	LOG("Load SM to f00d..\n");
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
	if (!lv0_sm_loaded) {
		LOG("Error: SM not loaded!\n");
		return -1;
	}
	int ret = nskbl_smtool_stop();
	LOG("nskbl_smtool_stop returned: 0x%08X\n", ret);
    v16p 0x51016cf2 = 0xf7ff;  // undo load_kprxauthsm patches
    v16p 0x51016cf4 = 0xf915;  // ^
    nskbl_clean_dcache((void *)0x51016cf0, 0x20);
    nskbl_flush_icache();
    v16p 0x51016bec = 0x6073;  // undo the 3rd arg patch
    nskbl_clean_dcache((void *)0x51016be0, 0x20);
    nskbl_flush_icache();
    lv0_sm_loaded = 0;
	return ret;;
}

int lv0_call_sm(int svc, void *argv, uint32_t size) {
	if (!lv0_sm_loaded) {
		LOG("Error: SM not loaded!\n");
		return -1;
	}
	if (!argv || !size) {
		LOG("Invalid arguments for SM call\n");
		return -1;
	}
	return nskbl_smtool_call_big_ka(argv, size + 0x40, svc);
}

static int lv0_ussm_corrupt(void *cmd_buf, uint32_t addr) {
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
    void *cmd_args = rmemblock_alloc(sizeof(struct lv0_corrupt_args_cmd_s), 0x10208006, 0);
    if (!cmd_args) {
		LOG("Failed to allocate jump command buffer\n");
		return -1;
	}
    void *payload_buf = rmemblock_alloc(LV0_DEFAULT_PAYLOAD_SIZE, 0x10208006, LV0_DEFAULT_PAYLOAD_PA);
    if (!payload_buf) {
		LOG("Failed to allocate payload buffer\n");
		my_free(cmd_args);
		return -1;
	}
    for (uint32_t addr = 0x0080bd10; addr < 0x0080bd24; addr += 4) {
        lv0_ussm_corrupt((void *)cmd_args, addr);  // patch ussm::0xd0002 to skip bound checks
    }
    uint8_t spr_backup[0x80];
    memcpy(spr_backup, payload_buf, sizeof(spr_backup));
    memset(payload_buf, 0, 0x300);
    memcpy(payload_buf, NMPstage2_payload, sizeof(NMPstage2_payload));
	memcpy(payload_buf + 0x100, inject_framework_nmp, sizeof(inject_framework_nmp));
	memcpy(payload_buf + 0x200, framework_nmp, sizeof(framework_nmp));
	struct lv0_jump_args_cmd_s *jumd = cmd_args;
	memset(jumd, 0, sizeof(struct lv0_jump_args_cmd_s));
	jumd->size = sizeof(struct lv0_jump_args_cmd_s) + 0x40;
	jumd->service_id = 0xd0002;
	jumd->req[0] = LV0_DEFAULT_PAYLOAD_PA;  // paddr of stage2
    int ret = lv0_call_sm(0xd0002, cmd_args, sizeof(struct lv0_jump_args_cmd_s));  // call_kprxauthsm_func
    LOG("lv0_call_sm ret=0x%08X\n", ret);
    memcpy(payload_buf, spr_backup, sizeof(spr_backup));
    my_free(payload_buf);
	my_free(cmd_args);
	return 0;
}

int lv0_initialized = 0;
int lv0_init(const char *ussm) {
	if (lv0_initialized) {
		LOG("lv0 is already initialized\n");
		return 0;
	}
    int ret = lv0_load_sm(ussm);
    if (ret) {
        LOG("Failed to load update SM: 0x%08X\n", ret);
		return ret;
    } else
		lv0_sm_loaded = 1;
    ret = lv0_ussm_install_spl();
	if (ret < 0)
		LOG("Failed to install lv0 SPL: 0x%08X\n", ret);
	else {
		LOG("lv0 SPL installed successfully\n");
		lv0_initialized = 1;
	}
	lv0_stop_sm();
	return ret;
}

int lv0_spl_exec(void *payload, uint32_t paddr, int size, uint32_t arg) {
	void *paddr_buf = NULL;
    void *payload_buf = rmemblock_alloc(LV0_DEFAULT_PAYLOAD_SIZE, 0x10208006, LV0_DEFAULT_PAYLOAD_PA);
    if (!payload_buf) {
        LOG("Failed to allocate payload buffer\n");
        return -1;
    }
	if (!paddr)
        paddr = LV0_SPL_PAYLOAD_PA;  // use default paddr if not provided
	if (payload) {
		if ((paddr >= LV0_DEFAULT_PAYLOAD_PA) && (paddr < (LV0_DEFAULT_PAYLOAD_PA + LV0_DEFAULT_PAYLOAD_SIZE))) {
			if (paddr < LV0_SPL_PAYLOAD_PA) {
				LOG("Invalid paddr for SPL payload: 0x%08X\n", paddr);
				my_free(payload_buf);
				return -1;
			} else
                memcpy(payload_buf, payload, size);
        } else {
            paddr_buf = rmemblock_alloc(size, 0x10208006, paddr);
            if (!paddr_buf) {
				LOG("Failed to allocate payload buffer at paddr 0x%08X\n", paddr);
				my_free(payload_buf);
				return -1;
			}
			memcpy(paddr_buf, payload, size);
		}
	}
    lv0_spl_fm_nfo *spl_nfo = (lv0_spl_fm_nfo *)payload_buf;
	memset(payload_buf, 0, sizeof(lv0_spl_fm_nfo));
	spl_nfo->magic = 0x14ff;
	spl_nfo->status = 0x34;
    spl_nfo->codepaddr = paddr;
	spl_nfo->arg = arg;
	spl_nfo->resp = 0;  // response will be filled by the SPL
	LOG("Executing SPL payload at paddr=0x%08X, size=%d, arg=0x%08X\n", paddr, size, arg);
	nskbl_smc_custom(0, 0, 0, 0, 0x13c);
    LOG("SPL execution completed with status: 0x%08X, response: 0x%08X\n", spl_nfo->status, spl_nfo->resp);
    int ret = (spl_nfo->status == 0x69) ? (int)spl_nfo->resp : -1;
	my_free(payload_buf);
	if (paddr_buf)
		my_free(paddr_buf);
	return ret;
}