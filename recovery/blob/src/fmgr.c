#include "stor.h"
#include "utils.h"
#include "view.h"
#include "paper.h"
#include "ff.h"
#include "bm_ext.h"

#include "fmgr.h"

static const char *fmgr_file_options[FMGR_FILE_OP_COUNT] = {
	"Copy to the other side",
	"Move to the other side",
	"Delete file"
};

static const char *fmgr_dir_options[FMGR_FILE_OP_COUNT] = {
	"Copy to the other side",
	"Move to the other side",
	"Delete directory"
};

static const char **fmgr_options_per_type[FMGR_ENTRY_TYPE_COUNT] = {
	[FMGR_ENTRY_TYPE_FILE] = fmgr_file_options,
	[FMGR_ENTRY_TYPE_DIR] = fmgr_dir_options,
	[FMGR_ENTRY_TYPE_MOUNT_ACTIVE] = NULL,
	[FMGR_ENTRY_TYPE_MOUNT_INACTIVE] = NULL,
	[FMGR_ENTRY_TYPE_PARTITION] = NULL,
	[FMGR_ENTRY_TYPE_OPTION] = NULL
};

static const char *fmgr_mount_names[STOR_MAX_MOUNTS] = {
	"mnt0:",
	"mnt1:",
};

static FATFS fmgr_mountp[2] = {
	{0},  // mnt0
	{0},  // mnt1
};

static struct paper_s fmgr_paper[FMGR_PAPERS_END];

static struct fmgr_xv_ctx_s {
    int active;
    int mount_idx;
    struct {
        enum FMGR_XV_LOCATIONS id;
        char cwd[FMGR_MAX_PATH_LEN];
        char entries[FMGR_XV_ENTRY_COUNT][FMGR_XV_MAX_NAME_LEN];
        int entry_count;  // number of entries in the current view
        int entry_start;  // start index for the current view
        int selection;
    } loc;
	int max_ent_count;
	int max_ent_len;
    struct paper_s *paper;
} fmgr_xv_ctx[2] = {
    [FMGR_PAPERS_LV] = {
		.active = 0,
        .mount_idx = 0,
        .loc = { .id = FMGR_XV_LOC_ROOT, .cwd = "/", .entry_count = 0, .entry_start = 0, .selection = 0},
		.max_ent_count = FMGR_XV_ENTRY_COUNT,
		.max_ent_len = FMGR_XV_MAX_NAME_LEN,
        .paper = &fmgr_paper[FMGR_PAPERS_LV]
    },
    [FMGR_PAPERS_RV] = {
        .active = 0,
		.loc = { .id = FMGR_XV_LOC_ROOT, .cwd = "/", .entry_count = 0, .entry_start = 0, .selection = 0},
        .mount_idx = 0,
        .max_ent_count = FMGR_XV_ENTRY_COUNT,
        .max_ent_len = FMGR_XV_MAX_NAME_LEN,
        .paper = &fmgr_paper[FMGR_PAPERS_RV]
    }
};

static struct fmgr_prev_xv_ctx_s {
	int uview;
	int selection;
    enum FMGR_ENTRY_TYPES sel_entype;  // ONLY SET IN OPTIONS VIEW
    struct paper_s *paper;
} fmgr_prev_xv_ctx = {
	.uview = FMGR_PAPERS_LV,
	.selection = 0,
	.sel_entype = FMGR_ENTRY_TYPE_FILE,
	.paper = &fmgr_paper[FMGR_PAPERS_LV]
};
static int fmgr_draw_context(int new_uview) {
	if (new_uview < FMGR_PAPERS_LV || new_uview > FMGR_PAPERS_RV) {
		LOG("Invalid view: %d\n", new_uview);
		return -1;
	}
    struct fmgr_xv_ctx_s *lctx = &fmgr_xv_ctx[FMGR_PAPERS_LV];
	struct fmgr_xv_ctx_s *rctx = &fmgr_xv_ctx[FMGR_PAPERS_RV];
	struct fmgr_xv_ctx_s *cctx = &fmgr_xv_ctx[new_uview];
	struct fmgr_prev_xv_ctx_s *pctx = &fmgr_prev_xv_ctx;

	// First, clear the previous selection
	int sel_ypos = pctx->selection * (pctx->paper->pen.width.y + pctx->paper->padding.inner.y)
					+ pctx->paper->padding.outer.y
					- pctx->paper->padding.inner.y;
	paper_draw_rectangle(pctx->paper, 0, sel_ypos,
						 pctx->paper->max.x - pctx->paper->min.x,
						 pctx->paper->pen.width.y + (2 * pctx->paper->padding.inner.y),
						 pctx->paper->color, pctx->paper->padding.inner.y >> 1);

    // Then draw the border arrows & selection for each view
    // Left view
	sel_ypos = lctx->loc.selection * (lctx->paper->pen.width.y + lctx->paper->padding.inner.y)
					+ lctx->paper->padding.outer.y
					- lctx->paper->padding.inner.y;
    if (new_uview == FMGR_PAPERS_LV) {
        paper_draw_rectangle(lctx->paper, 0, sel_ypos, lctx->paper->max.x - lctx->paper->min.x,
                             lctx->paper->pen.width.y + (2 * lctx->paper->padding.inner.y), FMGR_LRV_SELECTOR_COLR,
                             lctx->paper->padding.inner.y >> 1);
    }
    paper_clear(&fmgr_paper[FMGR_PAPERS_LB], fmgr_paper[FMGR_PAPERS_LB].color);
    pen_pos(&fmgr_paper[FMGR_PAPERS_LB], fmgr_paper[FMGR_PAPERS_LB].padding.outer.x, sel_ypos + lctx->paper->padding.inner.y);
	paper_write(&fmgr_paper[FMGR_PAPERS_LB], ">", 1);
	// Right view
	sel_ypos = rctx->loc.selection * (rctx->paper->pen.width.y + rctx->paper->padding.inner.y)
					+ rctx->paper->padding.outer.y
					- rctx->paper->padding.inner.y;
    if (new_uview == FMGR_PAPERS_RV) {
        paper_draw_rectangle(rctx->paper, 0, sel_ypos, rctx->paper->max.x - rctx->paper->min.x,
                             rctx->paper->pen.width.y + (2 * rctx->paper->padding.inner.y), FMGR_LRV_SELECTOR_COLR,
                             rctx->paper->padding.inner.y >> 1);
    }
    paper_clear(&fmgr_paper[FMGR_PAPERS_RB], fmgr_paper[FMGR_PAPERS_RB].color);
	pen_pos(&fmgr_paper[FMGR_PAPERS_RB], fmgr_paper[FMGR_PAPERS_RB].padding.outer.x, sel_ypos + rctx->paper->padding.inner.y);
	paper_write(&fmgr_paper[FMGR_PAPERS_RB], "<", 1);

	// Finally write out the info of the current view
	paper_clear(&fmgr_paper[FMGR_PAPERS_IB], fmgr_paper[FMGR_PAPERS_IB].color);
	pen_reset(&fmgr_paper[FMGR_PAPERS_IB], FMGR_IB_PEN_COLR);
    if ((lctx->loc.id == FMGR_XV_LOC_ROOT) && (rctx->loc.id == FMGR_XV_LOC_ROOT))
        pprintf_align(&fmgr_paper[FMGR_PAPERS_IB], CENTER, ">>> Press SELECT to (re)initialize all storage devices <<<");
	else
    	pprintf_align(&fmgr_paper[FMGR_PAPERS_IB], CENTER, "Path: /%s", fmgr_xv_ctx[new_uview].loc.cwd);

	// Update the previous context
	pctx->uview = new_uview;
	pctx->selection = cctx->loc.selection;
	pctx->paper = cctx->paper;
    return 0;
}

static int fmgr_list_dir(const char *path, char *output, int entry_len, int start, int max) {
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

        if (strlen(fno.fname) > entry_len - 1) {
            LOG("Filename too long: %s\n", fno.fname);
			//continue;  // skip long filenames
        }

        if (fno.fattrib & AM_DIR)
			my_snprintf(output + (count * entry_len), entry_len, "%s/", fno.fname);
		else
        	my_snprintf(output + (count * entry_len), entry_len, "%s", fno.fname);
		output[(count * entry_len) + entry_len - 1] = '\0';  // ensure null termination

		count++;
    }
	f_closedir(&dir);
	return count;
}

static const char *mount_master_names[] = {
	[MOUNT_MASTER_EMMC] = "eMMC",
	[MOUNT_MASTER_GCSD] = "GC-SD"
};

static int fmgr_scan_masters(char *output_s, int entry_len, uint32_t *output_i, int start, int max) {
    LOG("fmgr_scan_masters(output_s=%08X, output_i=%08X, entry_len=%d, start=%d, max=%d)\n", output_s, output_i, entry_len, start, max);
    int count = 0;
	int started = !start; // welp, who cares, memory is free anyways
	max = start + max;
	uint32_t parts = 0;
	for (int i = (MOUNT_MASTER_COUNT - 1); i >= 0; i--) {
		parts = 0;
		enum MOUNT_MASTER_TYPES type = stor_get_master_info(i, &parts);
		if (type == MOUNT_MASTER_TYPE_NONE)
			continue;  // skip uninitialized masters
		if (type == MOUNT_MASTER_TYPE_FAT) {
            if (started) {
                if (output_s)
					my_snprintf(output_s + (count * entry_len), entry_len, "%s -> entire", mount_master_names[i]);
				if (output_i)
					output_i[count - start] = FMGR_MASTER_SCAN_PACK(i, STOR_PART_ENTIRE, STOR_PART_ACTIVE_BOTH);
            }
            count++;
		} else if (type == MOUNT_MASTER_TYPE_SCE) {
			for (int j = 1; j < 16; j++) {
				if (parts & BITN(j)) {
                    if (started) {
                        if (output_s)
                        	my_snprintf(output_s + (count * entry_len), entry_len, "%s -> %s  (ina)", mount_master_names[i], get_partition_name(j));
						if (output_i)
							output_i[count - start] = FMGR_MASTER_SCAN_PACK(i, j, STOR_PART_ACTIVE_NOT);
                    }
                    count++;
					if (started && count >= max)
						return count - start;
                    if (count == start)
                        started = 1;
                }
				if (parts & BITN(j + 16)) {
                    if (started) {
						if (output_s)
                        	my_snprintf(output_s + (count * entry_len), entry_len, "%s -> %s  (act)", mount_master_names[i], get_partition_name(j));
						if (output_i)
							output_i[count - start] = FMGR_MASTER_SCAN_PACK(i, j, STOR_PART_ACTIVE_YES);
					}
                    count++;
                    if (started && count >= max)
						return count - start;
                    if (count == start)
                        started = 1;
                }
			}
		}
		if (started && count >= max)
			return count - start;
        if (count == start)
            started = 1;  // we just reached the start
    }
	return count - start;
}

static int fmgr_print_loc(struct fmgr_xv_ctx_s *ctx) {
	if (!ctx || !ctx->active || !ctx->paper) {
		LOG("Invalid context\n");
		return -1;
	}
	char *entries = ctx->loc.entries[0];
	char cwp[FMGR_MAX_PATH_LEN + 8];
	ctx->loc.entry_count = 0;

	switch (ctx->loc.id) {
		case FMGR_XV_LOC_ROOT:
            for (int i = 0; i < STOR_MAX_MOUNTS; i++) {
                if (!stor_ff_init_mount(i))
                    my_snprintf(entries + (ctx->loc.entry_count * ctx->max_ent_len), ctx->max_ent_len, "mnt%d:/", i);
                else
                    my_snprintf(entries + (ctx->loc.entry_count * ctx->max_ent_len), ctx->max_ent_len, "mnt%d:", i);
                ctx->loc.entry_count++;
            }
            my_snprintf(cwp, sizeof(cwp), "/");
			break;
		case FMGR_XV_LOC_PARTITIONS:
			ctx->loc.entry_count = fmgr_scan_masters(entries, ctx->max_ent_len, NULL, 0, ctx->max_ent_count);
            my_snprintf(cwp, sizeof(cwp), "mnt%d:", ctx->mount_idx);
			break;
		case FMGR_XV_LOC_MOUNT:
		case FMGR_XV_LOC_DIR:
            my_snprintf(cwp, sizeof(cwp), "%s", ctx->loc.cwd);
            ctx->loc.entry_count = fmgr_list_dir(cwp, entries, ctx->max_ent_len, ctx->loc.entry_start, ctx->max_ent_count);
			break;
		case FMGR_XV_LOC_OPTS:
			for (int i = 0; i < FMGR_FILE_OP_COUNT; i++) {
                my_snprintf(entries + (ctx->loc.entry_count * ctx->max_ent_len), ctx->max_ent_len, "%s", fmgr_options_per_type[fmgr_prev_xv_ctx.sel_entype][i]);
                ctx->loc.entry_count++;
			}
			my_snprintf(cwp, sizeof(cwp), "Options");
			break;
		default:
			LOG("Invalid location ID: %d\n", ctx->loc.id);
			return -1;
    }

    if (ctx->loc.entry_count < 0) {
        LOG("Failed to list location %s\n", cwp);
        return -1;
    }

	paper_clear(ctx->paper, ctx->paper->color);
	pen_reset(ctx->paper, ctx->paper->pen.color);
    for (int i = 0; i < ctx->loc.entry_count; i++) {
        char *str = entries + (i * ctx->max_ent_len);
        if (HAS_ENDSLASH(str))
			pprintf_color(ctx->paper, CYAN, "%s\n", str);
		else
        	pprintf(ctx->paper, "%s\n", str);
    }
    return 0;
}

static int fmgr_init_xv(int side) {
	if (side < 0 || side > FMGR_PAPERS_RV) {
		LOG("Invalid side index: %d\n", side);
		return -1;
	}
	struct fmgr_xv_ctx_s *ctx = &fmgr_xv_ctx[side];
	ctx->active = 1;
    ctx->mount_idx = STOR_MAX_MOUNTS;  // nomount
	memset(ctx->loc.cwd, 0, sizeof(ctx->loc.cwd));
	ctx->loc.id = FMGR_XV_LOC_ROOT;
	ctx->loc.cwd[0] = '/';  // whatever
	ctx->loc.entry_count = 0;
	ctx->loc.entry_start = 0;
	ctx->loc.selection = 0;
    ctx->max_ent_count = FMGR_XV_ENTRY_COUNT;
	ctx->max_ent_len = FMGR_XV_MAX_NAME_LEN;
    ctx->paper = &fmgr_paper[side];
	paper_clear(ctx->paper, ctx->paper->color);
	pen_reset(ctx->paper, ctx->paper->pen.color);
	return fmgr_print_loc(ctx);
}

static enum FMGR_ENTRY_TYPES fmgr_guess_entype(struct fmgr_xv_ctx_s *ctx) {
	if (!ctx || !ctx->active || !ctx->paper) {
		LOG("Invalid context for guessing entry type\n");
		return FMGR_ENTRY_TYPE_FILE;  // default to file
	}
    enum FMGR_ENTRY_TYPES s_type = HAS_ENDSLASH(ctx->loc.entries[ctx->loc.selection]);
    if (s_type == FMGR_ENTRY_TYPE_FILE) {  // it can also be an inactive mountpoint of master's partition
        if (ctx->loc.id == FMGR_XV_LOC_ROOT)
            s_type = FMGR_ENTRY_TYPE_MOUNT_INACTIVE;
        else if (ctx->loc.id == FMGR_XV_LOC_PARTITIONS)
            s_type = FMGR_ENTRY_TYPE_PARTITION;
        else if (ctx->loc.id == FMGR_XV_LOC_OPTS)
            s_type = FMGR_ENTRY_TYPE_OPTION;
    } else if (ctx->loc.id == FMGR_XV_LOC_ROOT)
        s_type = FMGR_ENTRY_TYPE_MOUNT_ACTIVE;
	
	LOG("Guessed entry type: %d for selection %d in location %d\n", s_type, ctx->loc.selection, ctx->loc.id);
	return s_type;
}

static void fmgr_handle_cross(void) {
    struct fmgr_prev_xv_ctx_s *pctx = &fmgr_prev_xv_ctx;
    struct fmgr_xv_ctx_s *cctx = &fmgr_xv_ctx[pctx->uview];
	if (!cctx || !cctx->active || !cctx->paper) {
		LOG("Invalid context for cross handling\n");
		return;
	}

    char tmp_path[FMGR_MAX_PATH_LEN];
    enum FMGR_ENTRY_TYPES s_type = fmgr_guess_entype(cctx);
    switch (s_type) {
        case FMGR_ENTRY_TYPE_FILE: {
            if (fmgr_list_dir(cctx->loc.cwd, tmp_path, FMGR_MAX_PATH_LEN, cctx->loc.selection + cctx->loc.entry_start, 1) < 0) {
                LOG("ERROR: Failed to single-list directory %s\n", cctx->loc.cwd);
                return;
            }
			if (HAS_ENDSLASH(tmp_path))
				goto actually_dir;  // it's a directory, name probably truncated
            LOG("Crossed a file: %s\n", tmp_path);
			if (strlen(cctx->loc.cwd) + strlen(tmp_path) >= FMGR_MAX_PATH_LEN) {
				LOG("ERROR: Path too long, cannot handle file %s\n", tmp_path);
				return;
			}
            memcpy(&cctx->loc.cwd[strlen(cctx->loc.cwd)], tmp_path, FMGR_MAX_PATH_LEN - strlen(cctx->loc.cwd));
			LOG("Loading options for file %s\n", cctx->loc.cwd);
			cctx->loc.id = FMGR_XV_LOC_OPTS;
			cctx->loc.entry_start = 0;
			if (fmgr_print_loc(cctx) < 0) {
				LOG("Failed to print options for file %s (??)\n", cctx->loc.cwd);
				return;
			}
			cctx->loc.selection = 0;
            pctx->sel_entype = s_type;
            fmgr_draw_context(pctx->uview);
			return;
        } break;
        case FMGR_ENTRY_TYPE_DIR:
			// Change the current directory to the selected one
			{
				if (fmgr_list_dir(cctx->loc.cwd, tmp_path, FMGR_MAX_PATH_LEN, cctx->loc.selection + cctx->loc.entry_start, 1) < 0) {
					LOG("ERROR: Failed to single-list directory %s\n", cctx->loc.cwd);
					return;
				}
actually_dir:
				if (strlen(cctx->loc.cwd) + strlen(tmp_path) >= FMGR_MAX_PATH_LEN) {
					LOG("ERROR: Path too long, cannot change directory to %s\n", tmp_path);
					cctx->loc.entry_start = 0;  // reset entry start
					fmgr_print_loc(cctx);
					return;
				}
                memcpy(&cctx->loc.cwd[strlen(cctx->loc.cwd)], tmp_path, FMGR_MAX_PATH_LEN - strlen(cctx->loc.cwd));
                cctx->loc.entry_start = 0;  // reset entry start
				LOG("Changing directory to %s\n", cctx->loc.cwd);
				cctx->loc.id = FMGR_XV_LOC_DIR;
				if (fmgr_print_loc(cctx) < 0) {
					LOG("Failed to print directory %s\n", cctx->loc.cwd);
					return;
				}
				cctx->loc.selection = 0;  // reset selection
				fmgr_draw_context(pctx->uview);
				return;
			}
			break; // lul
        case FMGR_ENTRY_TYPE_PARTITION:
            // Try to mount the partition
            {
                uint32_t scan_results = 0;
                int ret = fmgr_scan_masters(NULL, 0, &scan_results, cctx->loc.selection, 1);
                if (ret != 1) {
                    LOG("Could not get partition info for selection %d: %d [%X]\n", cctx->loc.selection, ret, scan_results);
                    return;
                }
                ret = stor_init_mount(cctx->mount_idx, FMGR_MASTER_SCAN_UNPACK(MASTER, scan_results), FMGR_MASTER_SCAN_UNPACK(PARTITION, scan_results),
                                      FMGR_MASTER_SCAN_UNPACK(ACTIVE, scan_results));
                if (ret < 0) {
                    LOG("Failed to init mount %d (%d, %d, %d): %d\n", cctx->mount_idx, FMGR_MASTER_SCAN_UNPACK(MASTER, scan_results),
                        FMGR_MASTER_SCAN_UNPACK(PARTITION, scan_results), FMGR_MASTER_SCAN_UNPACK(ACTIVE, scan_results), ret);
                    return;
                }
                cctx->loc.cwd[4] = '\0';  // reset cwd to 'mntX'
                FRESULT res = f_mount(&fmgr_mountp[cctx->mount_idx], fmgr_mount_names[cctx->mount_idx], 1);
                if (res != FR_OK) {
                    LOG("Failed to f_mount %s: %d\n", fmgr_mount_names[cctx->mount_idx], res);
					stor_umount(cctx->mount_idx);  // unmount the partition
                    return;
                }
                cctx->loc.selection = cctx->mount_idx;  // reset selection
                LOG("Mounted partition %s to mnt%d successfully.\n", fmgr_mount_names[cctx->mount_idx], cctx->mount_idx);
				s_type = FMGR_ENTRY_TYPE_MOUNT_ACTIVE;  // change type to mount active
            } // fall through to mount active/inactive handling
        case FMGR_ENTRY_TYPE_MOUNT_ACTIVE:
        case FMGR_ENTRY_TYPE_MOUNT_INACTIVE:
            // change current dir to the mount point
			{
                cctx->mount_idx = cctx->loc.selection;
                memset(cctx->loc.cwd, 0, sizeof(cctx->loc.cwd));
				my_snprintf(cctx->loc.cwd, sizeof(cctx->loc.cwd), "mnt%d:/", cctx->mount_idx);
				cctx->loc.entry_start = 0;
				LOG("Entering mount %d: %s\n", cctx->mount_idx, cctx->loc.cwd);
				if (s_type == FMGR_ENTRY_TYPE_MOUNT_ACTIVE)
					cctx->loc.id = FMGR_XV_LOC_MOUNT;  // mount active
				else
					cctx->loc.id = FMGR_XV_LOC_PARTITIONS;  // mount inactive, show partitions
				if (fmgr_print_loc(cctx) < 0) {
					LOG("Failed to print mount directory %s\n", cctx->loc.cwd);
					return;
				}
				cctx->loc.selection = 0;  // reset selection
				fmgr_draw_context(pctx->uview);
                return;
			}
			break;
		case FMGR_ENTRY_TYPE_OPTION:
			{
				LOG("Selected option: %s\n", fmgr_options_per_type[pctx->sel_entype][cctx->loc.selection]);
				return;  // do nothing for now
			}
			break;
	}

	LOG("Unhandled entry type: %d\n", s_type);
}

static struct fmgr_chandler_s {
	enum FMGR_ENTRY_TYPES exp_entype;
    void (*fmgr_custom_handler)(char *path, char *entry);
} fmgr_chandler = {
	.exp_entype = FMGR_ENTRY_TYPE_FILE,
	.fmgr_custom_handler = NULL,
};

int fmgr_set_square_handler(enum FMGR_ENTRY_TYPES exp_entype, void (*handler)(char *path, char *entry)) {
    fmgr_chandler.exp_entype = exp_entype;
	fmgr_chandler.fmgr_custom_handler = handler;
	LOG("Square handler set for entry type %d: %p\n", exp_entype, handler);
	return 0;
}

static void fmgr_handle_square(void) {
    struct fmgr_prev_xv_ctx_s *pctx = &fmgr_prev_xv_ctx;
    struct fmgr_xv_ctx_s *cctx = &fmgr_xv_ctx[pctx->uview];
    if (!cctx || !cctx->active || !cctx->paper) {
        LOG("Invalid context for triangle handling\n");
        return;
    }
	if (!fmgr_chandler.fmgr_custom_handler) {
		LOG("No custom handler set for square\n");
		return;
	}
    enum FMGR_ENTRY_TYPES s_type = fmgr_guess_entype(cctx);
    if (s_type != fmgr_chandler.exp_entype) {
        LOG("Expected entry type %d, but got %d\n", fmgr_chandler.exp_entype, s_type);
        return;
    }
    char tmp_path[FMGR_MAX_PATH_LEN];
    if (fmgr_list_dir(cctx->loc.cwd, tmp_path, FMGR_MAX_PATH_LEN, cctx->loc.selection + cctx->loc.entry_start, 1) < 0) {
        LOG("ERROR: Failed to single-list directory %s\n", cctx->loc.cwd);
        return;
    }
    LOG("Passing selected path to square handler: %s%s\n", cctx->loc.cwd, tmp_path);
    fmgr_chandler.fmgr_custom_handler(cctx->loc.cwd, tmp_path);
    return;  // custom handler took care of it
}

static void fmgr_handle_triangle(void) {
    struct fmgr_prev_xv_ctx_s *pctx = &fmgr_prev_xv_ctx;
    struct fmgr_xv_ctx_s *cctx = &fmgr_xv_ctx[pctx->uview];
    if (!cctx || !cctx->active || !cctx->paper) {
        LOG("Invalid context for triangle handling\n");
        return;
    }
    enum FMGR_ENTRY_TYPES s_type = fmgr_guess_entype(cctx);
    pctx->sel_entype = s_type;
    if (!fmgr_options_per_type[s_type]) {
		LOG("No options available for entry type %d\n", s_type);
		return;
	}
    char tmp_path[FMGR_MAX_PATH_LEN];
    if (fmgr_list_dir(cctx->loc.cwd, tmp_path, FMGR_MAX_PATH_LEN, cctx->loc.selection + cctx->loc.entry_start, 1) < 0) {
		LOG("ERROR: Failed to single-list directory %s\n", cctx->loc.cwd);
		return;
	}
    if (strlen(cctx->loc.cwd) + strlen(tmp_path) >= FMGR_MAX_PATH_LEN) {
        LOG("ERROR: Path too long, cannot handle %s\n", tmp_path);
        return;
    }
	memcpy(&cctx->loc.cwd[strlen(cctx->loc.cwd)], tmp_path, FMGR_MAX_PATH_LEN - strlen(cctx->loc.cwd));
    LOG("Loading options for file %s\n", cctx->loc.cwd);
    cctx->loc.id = FMGR_XV_LOC_OPTS;
    cctx->loc.entry_start = 0;
    if (fmgr_print_loc(cctx) < 0) {
        LOG("Failed to print options for file %s (??)\n", cctx->loc.cwd);
        return;
    }
    cctx->loc.selection = 0;
    fmgr_draw_context(pctx->uview);
    return;
}

static void fmgr_handle_select(void) {
    struct fmgr_xv_ctx_s *lctx = &fmgr_xv_ctx[FMGR_PAPERS_LV];
    struct fmgr_xv_ctx_s *rctx = &fmgr_xv_ctx[FMGR_PAPERS_RV];
	if (!(lctx->mount_idx >= STOR_MAX_MOUNTS) || !(rctx->mount_idx >= STOR_MAX_MOUNTS))
		return; // can only reinitialize if both views are in root view
	LOG("Switching user view to log\n");
    view_switch(VIEW_DEFAULT);
	LOG("Stopping all FF mounts...\n");
    f_unmount(fmgr_mount_names[0]);
    f_unmount(fmgr_mount_names[1]);

	LOG("Stopping all STOR mounts...\n");
	for (int i = 0; i < STOR_MAX_MOUNTS; i++) {
		if (stor_ff_init_mount(i) < 0)
			continue;  // skip uninitialized mounts
		stor_umount(i);
	}

	LOG("Initializing physical storage devices...\n");
	stor_init_master(MOUNT_MASTER_EMMC);
	stor_init_master(MOUNT_MASTER_GCSD);

	LOG("Redrawing contexts...\n");
    fmgr_init_xv(FMGR_PAPERS_LV);
    fmgr_init_xv(FMGR_PAPERS_RV);
    fmgr_draw_context(FMGR_PAPERS_LV);

    LOG("All done, switching back to file manager view...\n");
	view_switch(VIEW_FMGR);
}

static void fmgr_handle_circle(void) {
    struct fmgr_prev_xv_ctx_s *pctx = &fmgr_prev_xv_ctx;
    struct fmgr_xv_ctx_s *cctx = &fmgr_xv_ctx[pctx->uview];
    if (!cctx || !cctx->active || !cctx->paper) {
        LOG("Invalid context for circle handling\n");
        return;
    }
	if (cctx->loc.id == FMGR_XV_LOC_ROOT)
		return;  // cannot go up from root directory

	if (cctx->loc.id == FMGR_XV_LOC_PARTITIONS) { // in partition view
        fmgr_init_xv(pctx->uview);  // why not
        fmgr_draw_context(pctx->uview);
		return;
    }

	char *cut = cctx->loc.cwd + strlen(cctx->loc.cwd) - 1 - ((cctx->loc.cwd[strlen(cctx->loc.cwd) - 1] == '/') ? 1 : 0);
    while (cut > cctx->loc.cwd && *cut != '/') { // find the last slash
        cut--;
    }
    if ((cut <= cctx->loc.cwd) || (strlen(cctx->loc.cwd) <= 4)) {
        fmgr_init_xv(pctx->uview);  // why not
		fmgr_draw_context(pctx->uview);
		return;
    }
    cut++;
	*cut = '\0';
	
	LOG("Going up to directory: %s\n", cctx->loc.cwd);
    cctx->loc.entry_start = 0;  // reset entry start
	cctx->loc.selection = 0;  // reset selection
	if (strlen(cctx->loc.cwd) == 6 && cctx->loc.cwd[4] == ':') {
		cctx->loc.id = FMGR_XV_LOC_MOUNT;
		LOG("Reached top of mount %s\n", cctx->loc.cwd);
	} else
		cctx->loc.id = FMGR_XV_LOC_DIR;
	if (fmgr_print_loc(cctx) < 0)
		LOG("Failed to print directory %s\n", cctx->loc.cwd);
	fmgr_draw_context(pctx->uview);

	return;
}

#define FMGR_NAV_BUTTONS (CTRL_UP | CTRL_DOWN | CTRL_LEFT | CTRL_RIGHT)
static void fmgr_handle_nav(int button) {
    struct fmgr_prev_xv_ctx_s *pctx = &fmgr_prev_xv_ctx;
	struct fmgr_xv_ctx_s *cctx = &fmgr_xv_ctx[pctx->uview];
	if (!cctx || !cctx->active || !cctx->paper) {
		LOG("Invalid context for navigation\n");
		return;
	}
	switch (button) {
		case CTRL_UP:
			if (cctx->loc.selection > 0) {
				cctx->loc.selection--;
				fmgr_draw_context(pctx->uview);
			} else if (cctx->loc.selection == 0 && cctx->loc.entry_start > 0) {
				cctx->loc.entry_start--;
				fmgr_print_loc(cctx);
				fmgr_draw_context(pctx->uview);
			}
			break;
		case CTRL_DOWN:
			if (cctx->loc.selection < cctx->loc.entry_count - 1) {
				cctx->loc.selection++;
				fmgr_draw_context(pctx->uview);
			} else if (cctx->loc.selection == FMGR_XV_ENTRY_COUNT - 1) {
				cctx->loc.entry_start++;
				fmgr_print_loc(cctx);
				fmgr_draw_context(pctx->uview);
			}
			break;
		case CTRL_RIGHT:
			if (pctx->uview == FMGR_PAPERS_LV)
				fmgr_draw_context(FMGR_PAPERS_RV);
			break;
		case CTRL_LEFT:
			if (pctx->uview == FMGR_PAPERS_RV)
				fmgr_draw_context(FMGR_PAPERS_LV);
			break;
		default:
			break;
	}
}

#define FMGR_ACTION_BUTTONS (CTRL_CROSS | CTRL_SELECT | CTRL_CIRCLE | CTRL_TRIANGLE | CTRL_SQUARE)
int fmgr_view_handler(int *next_uview) {
    view_switch(VIEW_FMGR);  // switch to the file manager view

    int buttons = 0;
    while (1) {
        // Wait for user input
        buttons = bmx_ctrl_wait(CTRL_R | CTRL_L | FMGR_ACTION_BUTTONS | FMGR_NAV_BUTTONS, 4000, 1);
        if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_R)) {
            LOG("Switching to log view...\n");
            *next_uview = VIEW_DEFAULT;
            return 0;
        } else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_L)) {
            LOG("Switching to menu view...\n");
            *next_uview = VIEW_MENU;
            return 0;
		} else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_DOWN)) // "wish there was a better way to do this" :P
			fmgr_handle_nav(CTRL_DOWN);
		else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_UP))
			fmgr_handle_nav(CTRL_UP);
		else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_RIGHT))
			fmgr_handle_nav(CTRL_RIGHT);
		else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_LEFT))
			fmgr_handle_nav(CTRL_LEFT);
		else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_CROSS))
            fmgr_handle_cross();
		else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_SELECT))
			fmgr_handle_select();
		else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_CIRCLE))
			fmgr_handle_circle();
		else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_TRIANGLE))
			fmgr_handle_triangle();
		else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_SQUARE))
			fmgr_handle_square();
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
    fmgr_init_xv(FMGR_PAPERS_LV);
    fmgr_init_xv(FMGR_PAPERS_RV);

    fmgr_draw_context(FMGR_PAPERS_LV);

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