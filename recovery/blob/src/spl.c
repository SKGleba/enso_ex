#include "spl.h"

#include "fmgr.h"
#include "utils.h"
#include "lv0.h"

static struct spl_import_s spl_imports;

static const char *spl_CL_dbgid = "[RSPL]";

static struct spl_selective_s spl_CL_selective = {
	.lv0_init = LSPL_CFGSEL_LV0_INIT,
	.lv0_deinit = LSPL_CFGSEL_LV0_DEINIT,
	.tzs_init = LSPL_CFGSEL_TZS_INIT,
	.tzs_deinit = LSPL_CFGSEL_TZS_DEINIT,
	.lv0p_nmp.va = NULL,
	.lv0p_nmp.size = 0,
};

static char *spl_ussm_path = NULL;
static int spl_CL_loadussm(void) {
	if (!spl_ussm_path) {
        ELOG("Error: SM path is not set!\n");
        return -1;
    }
	return lv0_load_sm(spl_ussm_path);
}

static void *spl_CL_palloc(uint32_t paddr, uint32_t size) {
	return rmemblock_alloc(size, MEMBLOCK_TYPE_UCRW, paddr);
}

static void *spl_CL_malloc(uint32_t size) {
	return rmemblock_alloc(size, MEMBLOCK_TYPE_UCRW, 0);
}

static void spl_CL_free(void *ptr) {
	my_free(ptr);
}

static uint32_t spl_CL_getfw(void) {
	return 0x03650000;
}

static int spl_CL_smcall(int idx, int a0, int a1, int a2, int a3) {
	return nskbl_smc_custom(a0, a1, a2, a3, idx);
}

static int spl_CL_stub(void) {
	return 0;
}

static void spl_dispatch(struct spl_import_s *sis) {
	sis->memset = memset;
	sis->memcpy = memcpy;
	sis->palloc = spl_CL_palloc;
	sis->pfree = spl_CL_free;
	sis->malloc = spl_CL_malloc;
	sis->free = spl_CL_free;
	sis->loadussm = spl_CL_loadussm;
	sis->callussm = lv0_call_sm;
	sis->unloadussm = lv0_stop_sm;
	sis->getfw = spl_CL_getfw;
	sis->smcall = spl_CL_smcall;
	sis->debug = nskbl_printf;
	sis->error = nskbl_printf;
}

int spl_NOTinitialized = 1;
static struct spl_init_arg_s spl_iarg;
int spl_init(const char *ussm_path, int flags) {
	DLOG("spl_init(ussm_path: %s, flags: %X)\n", ussm_path, flags);
	spl_NOTinitialized = 1;
	spl_ussm_path = (char *)ussm_path;
	memset(&spl_imports, 0, sizeof(struct spl_import_s));
	spl_dispatch(&spl_imports);
	spl_iarg.bufC0 = NULL;
	spl_iarg.imports = &spl_imports;
	spl_iarg.ifl = flags;
	spl_iarg.lv0_backup_mode = SPLV0_COMMBACKUP_NEVER;
	spl_iarg.dbgid = spl_CL_dbgid;
	spl_iarg.fw_override = 0x03650000;
	spl_CL_selective.lv0p_nmp.va = LSPL_CFGSEL_LV0P_NMP_VA;
	spl_CL_selective.lv0p_nmp.size = LSPL_CFGSEL_LV0P_NMP_SIZE;
	spl_iarg.sel = &spl_CL_selective;
	int ret = SPLx_INIT(&spl_iarg);
	DLOG("SPLx_INIT returned %d\n", ret);
	if (ret >= 0)
		spl_NOTinitialized = 0;
	return ret;
}

int spl_deinit(int flags) {
	spl_iarg.ifl = flags;
	int ret = SPLx_DEINIT(&spl_iarg);
	DLOG("SPLx_DEINIT returned %d\n", ret);
	memset(&spl_iarg, 0, sizeof(struct spl_init_arg_s));
	spl_NOTinitialized = 1;
	return ret;
}

int spl_lv0_exec(void *payload, int size, uint32_t pa_pa, uint32_t stack, int a0, int a1, int a2, int a3) {
	struct splv0_exec_arg_s argv = {
		.payload = payload,
		.size = size,
		.pa_pa = pa_pa,
		.stack = stack,
		.wait = false,
		.arg = {(uint32_t)a0, (uint32_t)a1, (uint32_t)a2, (uint32_t)a3},
		.xret = 0,
	};
	int ret = SPLx_LV0_EXEC(&argv, SPLV0_COMMBACKUP_NEVER);
	if (ret < 0)
		ELOG("SPLx_LV0_EXEC failed with error %d\n", ret);
	else {
		ret = argv.xret;
		ILOG("SPLx_LV0_EXEC succeeded with ret 0x%08X\n", ret);
	}
	return ret;
}

int spl_lv0p_run(struct splv0p_arg_s *argv, enum CHAIN_FREE_TYPES free) {
	if (!argv || !argv->gsize) {
        ELOG("Invalid splv0p arg struct\n");
        return -1;
    }
	ILOG("SPLx_LV0P(0x%08X(0x%08X), %d)\n", argv, argv->j, free);
	argv->nmp.va = LSPL_CFGSEL_LV0P_NMP_VA;
	argv->nmp.size = LSPL_CFGSEL_LV0P_NMP_SIZE;
	int ret = SPLx_LV0P(argv, SPLV0_COMMBACKUP_NEVER);
	if (ret < 0)
        ELOG("SPLx_LV0P failed with error code %d\n", ret);
	if (free == CHAIN_FREE_NONE)
		return ret;

	struct splv0p_j_s *j = argv->j;
	struct splv0p_j_s *nj = NULL;
	while (j) {
		nj = j->next;
		if (free & CHAIN_FREE_NESTED) {
			if ((j->idx == LV0P_JOB_DAT) && !j->d.ncopyin && j->d.src_va)
				my_free(j->d.src_va);
			else if ((j->idx == LV0P_JOB_EXE) && j->x.size && j->x.src_va)
				my_free(j->x.src_va);
		}
		if (free & CHAIN_FREE_ENTRIES)
			my_free(j);
		j = nj;
	}
	return ret;
}
