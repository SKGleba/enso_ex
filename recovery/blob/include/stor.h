#ifndef __SDIF_H__
#define __SDIF_H__

#include <stdint.h>
#include "nskbl.h"

#define SECTOR_SIZE 0x200
#define SBLS_START 0x4000
#define SBLS_END 0x8000
#define IDSTOR_START 0x200

struct _partition_t {
    uint32_t off;
    uint32_t sz;
    uint8_t code;
    uint8_t type;
    uint8_t active;
    uint32_t flags;
    uint16_t unk;
} __attribute__((packed));
typedef struct _partition_t partition_t;

struct _master_block_t {
    char magic[0x20];
    uint32_t version;
    uint32_t device_size;
    uint32_t unused[2];
    uint32_t sl_off;
    uint32_t sl_sz;
    uint32_t active_sbls_off;
    uint32_t sbls_off[2];
    uint32_t active_os_off;
    uint32_t unused2[2];
    partition_t partitions[0x10];
    char unk2[0x5e];
    char unk3[0x10 * 4];
    uint16_t sig;
} __attribute__((packed));
typedef struct _master_block_t master_block_t;

extern int g_disable_bootarea_update;

int write_sector_sd(int *part_ctx, uint32_t sector, const void *buffer, int nsectors);
int write_sector_mmc(int *ctx, unsigned int block_offset, const void *target_buf, int block_count);
int sd_init(int tries, int init_sd0_part);

enum MOUNT_MASTERS {
    MOUNT_MASTER_EMMC = 0,
    MOUNT_MASTER_GCSD,
    MOUNT_MASTER_COUNT
};

enum MOUNT_MASTER_TYPES {
    MOUNT_MASTER_TYPE_NONE = 0,
    MOUNT_MASTER_TYPE_SCE,
    MOUNT_MASTER_TYPE_FAT,
};

struct mount_master_ctx {
    enum MOUNT_MASTER_TYPES type;
    int *dev_ctx;
    int (*read_sector)(int *ctx, uint32_t sector, void *buffer, int nsectors);
    int (*write_sector)(int *ctx, uint32_t sector, const void *buffer, int nsectors);
    master_block_t sector0;
};

struct mount_ctx {
    int is_initialized;
    enum MOUNT_MASTERS mount_master;
    struct mount_master_ctx *master;
    partition_t *params;
};

const char *get_partition_name(int part);

#define SCEMBR_U32_MAGIC 'ynoS'
#define FAT_MBR_MAGIC 0xAA55

#define IS_GCSD_INITIALIZED() (!!(*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX))

#define STOR_MAX_MOUNTS 3 // two main, one temp

enum STOR_PARTITIONS {
    STOR_PART_ENTIRE = 0,
    STOR_PART_IDSTOR,
    STOR_PART_SLOADER,
    STOR_PART_OS,
    STOR_PART_VSH,
    STOR_PART_VSHDATA,
    STOR_PART_VTRM,
    STOR_PART_USER,
    STOR_PART_USEREXT,
    STOR_PART_GAMERO,
    STOR_PART_GAMERW,
    STOR_PART_UPDATER,
    STOR_PART_SYSDATA,
    STOR_PART_MEDIAID,
    STOR_PART_PIDATA,
    STOR_PART_UNUSED
};

enum STOR_PART_ACTIVES {
    STOR_PART_ACTIVE_NOT = 0,
    STOR_PART_ACTIVE_YES,
    STOR_PART_ACTIVE_BOTH
};

enum MOUNT_MASTER_TYPES stor_init_master(enum MOUNT_MASTERS mount_master);
int stor_init_mount(int idx, enum MOUNT_MASTERS mount_master, enum STOR_PARTITIONS partition_id, enum STOR_PART_ACTIVES active);
int stor_ff_init_mount(int idx);
int stor_read_mount(int idx, uint32_t sector, void *buffer, int nsectors);
int stor_write_mount(int idx, uint32_t sector, const void *buffer, int nsectors);
enum MOUNT_MASTER_TYPES stor_get_master_info(enum MOUNT_MASTERS mount_master, uint32_t *partitions);
int stor_umount(int idx);
partition_t *stor_find_partition_by_id(master_block_t *master, int part_id, enum STOR_PART_ACTIVES active);
int stor_write_master(enum MOUNT_MASTERS master, uint32_t sector, const void *buffer, int nsectors);
int stor_read_master(enum MOUNT_MASTERS master, uint32_t sector, void *buffer, int nsectors);
struct mount_ctx *stor_get_validate_mctx(int idx);

#endif