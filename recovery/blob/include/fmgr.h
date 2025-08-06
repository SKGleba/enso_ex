#ifndef __FMGR_H__
#define __FMGR_H__

#include <baremetal/draw.h>

#include "utils.h"
#include "main.h"

enum FMGR_PAPERS {
	FMGR_PAPERS_LV = 0, // left panel
	FMGR_PAPERS_RV, // right panel
	FMGR_PAPERS_IB, // info panel
	FMGR_PAPERS_LB, // left border
	FMGR_PAPERS_RB, // right border
	FMGR_PAPERS_END
};

enum FMGR_MAIN_VIEWS_PARAMS {
    FMGR_LRV_PAPER_COLR = RGBA8(0x20, 0x20, 0x20, 0xFF),
    FMGR_LRV_PEN_COLR = WHITE,
    FMGR_LRV_SELECTOR_COLR = GREEN,
    FMGR_LRV_PAPER_START_Y = 60,
    FMGR_LRV_PAPER_END_Y = 500,
    FMGR_LRV_PAPER_INNER_PADDING_Y = 6,  // px between menu items (for the selector)
    FMGR_LV_PAPER_START_X = 20,
    FMGR_LV_PAPER_END_X = 470,
    FMGR_RV_PAPER_START_X = 490,
    FMGR_RV_PAPER_END_X = 940,
	FMGR_LRV_PAPER_OPAD_X = 8,
};

enum FMGR_BORDER_VIEWS_PARAMS {
    FMGR_ILRB_PAPER_COLR = BG_PAPER_COLR,
    FMGR_LRB_PEN_COLR = GREEN,
    FMGR_IB_PEN_COLR = YELLOW,
    FMGR_ILB_PAPER_START_X = 0,
    FMGR_LB_PAPER_END_X = FMGR_LV_PAPER_START_X,
    FMGR_IB_PAPER_END_X = 960,
    FMGR_RB_PAPER_START_X = FMGR_RV_PAPER_END_X,
    FMGR_RB_PAPER_END_X = 960,
    FMGR_LRB_PAPER_START_Y = FMGR_LRV_PAPER_START_Y,
    FMGR_LRB_PAPER_END_Y = FMGR_LRV_PAPER_END_Y,
    FMGR_IB_PAPER_START_Y = FMGR_LRV_PAPER_END_Y,
    FMGR_IB_PAPER_END_Y = 544,
	FMGR_LRB_PAPER_OPAD_X = 0,
};

enum FMGR_ENTRY_TYPES {
    FMGR_ENTRY_TYPE_FILE = 0,
    FMGR_ENTRY_TYPE_DIR,
    FMGR_ENTRY_TYPE_MOUNT_ACTIVE,
    FMGR_ENTRY_TYPE_MOUNT_INACTIVE,
    FMGR_ENTRY_TYPE_PARTITION,
    FMGR_ENTRY_TYPE_OPTION,
    FMGR_ENTRY_TYPE_COUNT
};

enum FMGR_XV_LOCATIONS {
    FMGR_XV_LOC_ROOT = 0,
    FMGR_XV_LOC_PARTITIONS,
    FMGR_XV_LOC_MOUNT,
    FMGR_XV_LOC_DIR,
    FMGR_XV_LOC_OPTS
};

enum FMGR_MASTER_SCAN_RESULTS {
    FMGR_MASTER_SCANNED_MASTER = 0,
    FMGR_MASTER_SCANNED_PARTITION = 8,
    FMGR_MASTER_SCANNED_ACTIVE = 16
};
#define FMGR_MASTER_SCAN_PACK(_master, _partition, _active) \
    (((_master) & 0xFF) | ((_partition) << FMGR_MASTER_SCANNED_PARTITION) | ((_active) << FMGR_MASTER_SCANNED_ACTIVE))
#define FMGR_MASTER_SCAN_UNPACK(_field, _res) \
    (((_res) >> FMGR_MASTER_SCANNED_##_field) & 0xFF)

#define FMGR_XV_MAX_NAME_LEN 27 // includes the null terminator
#define FMGR_XV_ENTRY_COUNT 18
#define FMGR_MAX_PATH_LEN 256

#define HAS_ENDSLASH(path) ((path)[strlen(path) - 1] == '/')

enum FMGR_FILE_OPTS {
    FMGR_FILE_OP_COPY = 0,
    FMGR_FILE_OP_MOVE,
    FMGR_FILE_OP_DELETE,
    FMGR_FILE_OP_EXECUTE,
    FMGR_FILE_OP_COUNT
};

enum FMGR_DIR_OPTS {
    FMGR_DIR_OP_COPY = 0,
    FMGR_DIR_OP_MOVE,
    FMGR_DIR_OP_DELETE,
    FMGR_DIR_OP_COUNT
};

enum FMGR_AMOUNT_OPTS {
    FMGR_AMOUNT_OP_REFRESH = 0,
    FMGR_AMOUNT_OP_UMOUNT,
    FMGR_AMOUNT_OP_REMOUNT,
    FMGR_AMOUNT_OP_COUNT
};

enum FMGR_IMOUNT_OPTS {
    FMGR_IMOUNT_OP_REFRESH = 0,
    FMGR_IMOUNT_OP_COUNT
};

enum FMGR_PART_OPTS {
    FMGR_PART_OP_DUMP = 0,
    FMGR_PART_OP_FLASH,
    FMGR_PART_OP_COUNT
};

#define FMGR_RAWDUMP_BLOCK_SECCOUNT (0x8000) // 16MB
#define FMGR_RAWDUMP_SEGMENT_BLKCOUNT (64) // 1GiB segments

int fmgr_move_dir(const char *src_path, const char *dest_path);
int fmgr_move_file(const char *src_path, const char *dest_path);
int fmgr_delete(const char *path);
uint32_t fmgr_copy_file(const char *src_path, const char *dest_path);
int fmgr_copy_dir(const char *src_path, const char *dest_path);
void *fmgr_get_file(const char *path, void *buf, int size, int offset);
int fmgr_raw_dump(uint32_t sector_start, uint32_t sector_count, const char *dest_dir);
int fmgr_scan_masters(char *output_s, int entry_len, uint32_t *output_i, int start, int max);
int fmgr_list_dir(const char *path, char *output, int entry_len, int start, int max);
int fmgr_load_exec(const char *path);
int fmgr_fd_partition(int is_flash, uint32_t part_info, char *dest_string);
int fmgr_init(void);
int fmgr_view_handler(enum VIEW_ASSIGNS *next_uview);
void *fmgr_square_handler(int set, enum FMGR_ENTRY_TYPES exp_entypes, void (*handler)(enum FMGR_ENTRY_TYPES entype, char *path, char *entry, enum VIEW_ASSIGNS *next_uview));

#endif