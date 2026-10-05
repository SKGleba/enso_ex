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
	if (ret >= 0)
		lv0_sm_loaded = 1;
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

int lv0_call_sm(void *argv) {
    struct lv0_cmd_s *cmd = (struct lv0_cmd_s *)argv;
    if (!argv || !cmd->size) {
		ELOG("Invalid arguments for SM call\n");
		return -1;
	}
    ILOG("lv0_call_sm(svc=0x%X, argv=%08X, size=0x%X)\n", cmd->service_id, argv, cmd->size);
	if (!lv0_sm_loaded) {
		ELOG("Error: SM not loaded!\n");
		return -1;
	}
    int ret = nskbl_smtool_call_big_ka(argv, cmd->size, cmd->service_id);
    DLOG("nskbl_smtool_call_big_ka returned: 0x%08X (rr=0x%08X)\n", ret, cmd->response);
	return ret;
}