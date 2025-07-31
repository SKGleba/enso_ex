#include "stor.h"
#include "utils.h"
#include "view.h"
#include "paper.h"
#include "ff.h"
#include "bm_ext.h"

#include "fmgr.h"

static struct paper_s fmgr_paper[FMGR_PAPERS_END];

static FATFS fmgr_mountp[2] = {
	{0},  // mnt0
	{0},  // mnt1
};

static struct fmgr_xv_ctx_s {
    int active;
    int mount_idx;
    struct {
        char cwd[FMGR_MAX_PATH_LEN];
        char entries[FMGR_XV_ENTRY_COUNT][FMGR_XV_MAX_NAME_LEN];
        int entry_count;  // number of entries in the current view
        int entry_start;  // start index for the current view
        int selection;
    } dir;
    struct paper_s *paper;
} fmgr_xv_ctx[2] = {
    [FMGR_PAPERS_LV] = {
		.active = 0,
        .mount_idx = 0,
        .dir = {.cwd = "/", .entry_count = 0, .entry_start = 0, .selection = 0},
        .paper = &fmgr_paper[FMGR_PAPERS_LV]
    },
    [FMGR_PAPERS_RV] = {
        .active = 0,
        .mount_idx = 0,
        .dir = {.cwd = "/", .entry_count = 0, .entry_start = 0, .selection = 0},
        .paper = &fmgr_paper[FMGR_PAPERS_RV]
    }
};

static int fmgr_list_dir(const char *path, char *output, int start, int max) {
	FRESULT res;
	DIR dir;
	FILINFO fno;
	res = f_opendir(&dir, path);
	if (res != FR_OK) {
		LOG("Failed to open directory %s: %d\n", path, res);
		return -1;
	}
	int count = 0;
	for (int i = 0; i < start; i++) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0) {
            f_closedir(&dir);
			LOG("Index %d out of bounds for directory %s\n", i, path);
			return 0;
        }
	}
	while (count < max) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK) {
			LOG("Failed to read directory %s: %d\n", path, res);
			f_closedir(&dir);
			return -1;
		}

		if (fno.fname[0] == 0)
			break;  // end of directory

        if (strlen(fno.fname) > FMGR_XV_MAX_NAME_LEN - 1) {
            LOG("Filename too long: %s\n", fno.fname);
			continue;  // skip long filenames
        }

        if (fno.fattrib & AM_DIR)
			snprintf(output + (count * FMGR_XV_MAX_NAME_LEN), FMGR_XV_MAX_NAME_LEN, "%s/", fno.fname);
		else
        	snprintf(output + (count * FMGR_XV_MAX_NAME_LEN), FMGR_XV_MAX_NAME_LEN, "%s", fno.fname);

		count++;
    }
	f_closedir(&dir);
	return count;
}

static int fmgr_print_dir(struct fmgr_xv_ctx_s *ctx) {
	if (!ctx || !ctx->active || !ctx->paper) {
		LOG("Invalid context\n");
		return -1;
	}
	char *output = ctx->dir.entries[0];
	char cwp[FMGR_MAX_PATH_LEN + 8];
	snprintf(cwp, sizeof(cwp), "mnt%d:%s", ctx->mount_idx, ctx->dir.cwd);
    int count = fmgr_list_dir(cwp, output, ctx->dir.entry_start, FMGR_XV_ENTRY_COUNT);
    if (count < 0) {
        LOG("Failed to list directory %s\n", cwp);
        return -1;
    }
	pen_reset(ctx->paper, ctx->paper->pen.color);
	pprintf_color(ctx->paper, CYAN, "../\n");
    for (int i = 0; i < count; i++) {
        char *str = output + (i * FMGR_XV_MAX_NAME_LEN);
        if (HAS_ENDSLASH(str))
			pprintf_color(ctx->paper, CYAN, "%s\n", str);
		else
        	pprintf(ctx->paper, "%s\n", str);
    }
    return 0;
}

static int fmgr_init_xv(int mount_idx, int side) {
	if (mount_idx < 0 || mount_idx >= ARRAYSIZE(fmgr_xv_ctx)) {
		LOG("Invalid mount index: %d\n", mount_idx);
		return -1;
	}
	if (side < 0 || side > FMGR_PAPERS_RV) {
		LOG("Invalid side index: %d\n", side);
		return -1;
	}
	struct fmgr_xv_ctx_s *ctx = &fmgr_xv_ctx[side];
	ctx->active = 1;
	ctx->mount_idx = mount_idx;
	snprintf(ctx->dir.cwd, sizeof(ctx->dir.cwd), "/");
	ctx->dir.entry_count = 0;
	ctx->dir.entry_start = 0;
	ctx->dir.selection = 0;
	ctx->paper = &fmgr_paper[side];
	return fmgr_print_dir(ctx);
}

void fmgr_init_mounts(void) {
    int ret;
    LOG("Initializing master...\n");
    ret = stor_init_master(MOUNT_MASTER_EMMC);
    if (ret < 0) {
        LOG("Failed to init eMMC: %d\n", ret);
    }

    LOG("Initializing mount 0...\n");
    ret = stor_init_mount(0, MOUNT_MASTER_EMMC, STOR_PART_OS, STOR_PART_ACTIVE_YES);
    if (ret < 0) {
        LOG("Failed to init mount 0: %d\n", ret);
    }

    FRESULT res = f_mount(&fmgr_mountp[0], "mnt0", 1);
	if (res != FR_OK) {
		LOG("Failed to mount mnt0: %d\n", res);
	} else {
		LOG("Mounted mnt0 successfully.\n");
	}
    fmgr_init_xv(0, FMGR_PAPERS_LV);
    fmgr_init_xv(0, FMGR_PAPERS_RV);
}

static int fmgr_current_uview = FMGR_PAPERS_LV;
static int fmgr_draw_context() {
	return 0;
}

int fmgr_view_handler(int *next_uview) {
    view_switch(VIEW_FMGR);  // switch to the file manager view

    int buttons = 0;
    while (1) {
        // Wait for user input
        buttons = bmx_ctrl_wait(CTRL_R | CTRL_L | CTRL_CROSS | CTRL_UP | CTRL_DOWN, 4000, 1);
        if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_R)) {
            LOG("Switching to log view...\n");
            *next_uview = VIEW_DEFAULT;
            return 0;
        } else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_L)) {
            LOG("Switching to menu view...\n");
            *next_uview = VIEW_MENU;
            return 0;
        } else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_CROSS)) {
            return 0;
        }
    }
    return 0;
}

static int fmgr_is_initialized = 0;
int fmgr_init(void) {
    if (fmgr_is_initialized) {
        LOG("File manager already initialized!\n");
		return 0;
    }
    for (int i = 0; i < FMGR_PAPERS_END; i++) {
        if (i < FMGR_PAPERS_LB)
            paper_clear(&fmgr_paper[i], fmgr_paper[i].color);
        pen_reset(&fmgr_paper[i], fmgr_paper[i].pen.color);
    }
	fmgr_is_initialized = 1;
	return 0;
}


// --- PAPERS
static struct paper_s fmgr_paper[FMGR_PAPERS_END] = {
	[FMGR_PAPERS_LV] = {
		.view_idx = VIEW_FMGR,
		.min = {FMGR_LV_PAPER_START_X, FMGR_LRV_PAPER_START_Y},
		.max = {FMGR_LV_PAPER_END_X, FMGR_LRV_PAPER_END_Y},
		.color = FMGR_LRV_PAPER_COLR,
		.pen = {
			.pos = {FMGR_LRV_PAPER_OPAD_X, DFL_PAPER_OPAD_Y},
			.width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
			.color = FMGR_LRV_PEN_COLR
		},
		.blank_mode = DFL_PAPER_BLANK_MODE,
		.align = PAPER_ALIGN_LEFT,
		.padding = {
			.inner = {DFL_PAPER_IPAD_X, FMGR_LRV_PAPER_INNER_PADDING_Y},
			.outer = {FMGR_LRV_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
		}
	},
	[FMGR_PAPERS_RV] = {
		.view_idx = VIEW_FMGR,
		.min = {FMGR_RV_PAPER_START_X, FMGR_LRV_PAPER_START_Y},
		.max = {FMGR_RV_PAPER_END_X, FMGR_LRV_PAPER_END_Y},
		.color = FMGR_LRV_PAPER_COLR,
		.pen = {
			.pos = {FMGR_LRV_PAPER_OPAD_X, DFL_PAPER_OPAD_Y},
			.width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
			.color = FMGR_LRV_PEN_COLR
		},
		.blank_mode = DFL_PAPER_BLANK_MODE,
		.align = PAPER_ALIGN_LEFT,
		.padding = {
			.inner = {DFL_PAPER_IPAD_X, FMGR_LRV_PAPER_INNER_PADDING_Y},
			.outer = {FMGR_LRV_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
		}
	},
	[FMGR_PAPERS_IB] = {
		.view_idx = VIEW_FMGR,
		.min = {FMGR_ILB_PAPER_START_X, FMGR_IB_PAPER_START_Y},
		.max = {FMGR_IB_PAPER_END_X, FMGR_IB_PAPER_END_Y},
		.color = FMGR_ILRB_PAPER_COLR,
		.pen = {
			.pos = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y},
			.width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
			.color = FMGR_IB_PEN_COLR
		},
		.blank_mode = DFL_PAPER_BLANK_MODE,
		.align = PAPER_ALIGN_LEFT,
		.padding = {
			.inner = {DFL_PAPER_IPAD_X, DFL_PAPER_IPAD_Y},
			.outer = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
		}
	},
	[FMGR_PAPERS_LB] = {
		.view_idx = VIEW_FMGR,
		.min = {FMGR_ILB_PAPER_START_X, FMGR_LRB_PAPER_START_Y},
		.max = {FMGR_LB_PAPER_END_X, FMGR_LRB_PAPER_END_Y},
		.color = FMGR_ILRB_PAPER_COLR,
		.pen = {
			.pos = {FMGR_LRB_PAPER_OPAD_X, DFL_PAPER_OPAD_Y},
			.width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
			.color = FMGR_LRB_PEN_COLR
		},
		.blank_mode = DFL_PAPER_BLANK_MODE,
		.align = PAPER_ALIGN_RIGHT,
		.padding = {
			.inner = {DFL_PAPER_IPAD_X, FMGR_LRV_PAPER_INNER_PADDING_Y},
			.outer = {FMGR_LRB_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
		}
	},
	[FMGR_PAPERS_RB] = {
		.view_idx = VIEW_FMGR,
		.min = {FMGR_RB_PAPER_START_X, FMGR_LRB_PAPER_START_Y},
		.max = {FMGR_RB_PAPER_END_X, FMGR_LRB_PAPER_END_Y},
		.color = FMGR_ILRB_PAPER_COLR,
		.pen = {
			.pos = {FMGR_LRB_PAPER_OPAD_X, DFL_PAPER_OPAD_Y},
			.width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
			.color = FMGR_LRB_PEN_COLR
		},
		.blank_mode = DFL_PAPER_BLANK_MODE,
		.align = PAPER_ALIGN_RIGHT,
		.padding = {
			.inner = {DFL_PAPER_IPAD_X, FMGR_LRV_PAPER_INNER_PADDING_Y},
			.outer = {FMGR_LRB_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
		}
	}
};