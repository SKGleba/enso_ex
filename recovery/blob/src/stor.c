#include <baremetal/gpio.h>
#include <baremetal/pervasive.h>
#include <baremetal/syscon.h>
#include <baremetal/sysroot.h>
#include <baremetal/utils.h>

#include "nskbl.h"
#include "utils.h"
#include "stor.h"

// patch read_sector_sd to send a write command instead
static void set_sd_write_mode(int mode) {
    if (mode) {
        *(uint8_t *)0x5101e8b7 = 0x62;
        *(uint8_t *)0x5101e8bb = 0x60;
        *(uint8_t *)0x5101e8c2 = 0x18;
        *(uint8_t *)0x5101e8c4 = 0x19;
        nskbl_clean_dcache((void *)0x5101e8b0, 0x20);
        nskbl_flush_icache();
    } else {  // undo patches
        *(uint8_t *)0x5101e8b7 = 0x52;
        *(uint8_t *)0x5101e8bb = 0x50;
        *(uint8_t *)0x5101e8c2 = 0x11;
        *(uint8_t *)0x5101e8c4 = 0x12;
        nskbl_clean_dcache((void *)0x5101e8b0, 0x20);
        nskbl_flush_icache();
    }
}

// xd
int write_sector_sd(int *part_ctx, uint32_t sector, const void *buffer, int nsectors) {
    conlog("WRITE to SD, sector: %u, nsectors: %d\n", sector, nsectors);
    set_sd_write_mode(1);
    int ret = read_sector_sd(part_ctx, sector, (const void *)buffer, nsectors);
    set_sd_write_mode(0);
    return ret;
}

// write [block_count] from [target_buf] to MMC @ [block_offset] for ctx [ctx]
static int write_sector_mmc_direct(int *ctx, unsigned int block_offset, const void *target_buf, int block_count) {
    int ret, unkcmd1, ctxs2, unkcmd3;
    ;
    uint32_t *ctxspav, *ctxspav_2;
    unsigned int *ctxdd, unkcmd0, unkcmd2, ctxdd_a, ctxdd_b, rand[4];
    long long int daddr;
    unsigned short ctx2s;

    if (((target_buf == NULL || ctx == NULL) || (block_count == 0)) || (ctxs2 = *ctx, ctxs2 == 0)) {
        ret = -1;
        goto EXIT;
    }

    ret = lsdif_mmc_verify_args(ctx, block_offset, block_count);

    if (ret < 0)
        goto EXIT;

    ctxspav = (uint32_t *)lsdif_mmc_prep_ctx(ctxs2);

    if (ctxspav == NULL) {
    RBAD_ARGS:
        ret = -2;
        goto EXIT;
    }

    if (block_count == 1)
        ret = 0x18;
    else
        ret = 0x19;

    unkcmd0 = ctx[1];
    *ctxspav = 0x240;
    ctxspav[1] = 0x614;
    ctxspav[2] = ret;
    ctx2s = *(unsigned short *)(ctx + 2);
    unkcmd2 = (uint32_t)ctx2s;
    ctxdd = (unsigned int *)ctx[0xe4];
    ctxdd_a = ctxdd[4];
    ctxdd_b = *ctxdd;
    daddr = (unsigned long long int)unkcmd2 * (unsigned long long int)block_offset +
            CONCAT44(ctxdd[5] - (ctxdd[1] + (uint32_t)(ctxdd_a < ctxdd_b)), ctxdd_a - ctxdd_b);
    unkcmd1 = lsysclib_concat_unk((int)daddr, (int)((unsigned long long int)daddr >> 0x20), unkcmd2, 0);

    if ((unkcmd0 & 1) == 0)
        unkcmd1 = unkcmd1 << 9;

    ctxspav[3] = unkcmd1;
    ctxspav[8] = (uint32_t)target_buf;
    *(unsigned short *)(ctxspav + 9) = ctx2s;
    *(uint16_t *)((int)ctxspav + 0x26) = (short)block_count;

    if (ret == 0x19) {
        ctxspav_2 = (uint32_t *)lsdif_mmc_prep_ctx(ctxs2);

        if (ctxspav_2 == NULL) {
            lsdif_mmc_write_args(ctxs2, ctxspav);
            goto RBAD_ARGS;
        }

        ctxspav_2[3] = block_count;
        *ctxspav_2 = 0x240;
        ctxspav_2[1] = 0x13;
        ctxspav_2[2] = 0x17;
        ctxspav_2[8] = 0;
        ret = lsdif_mmc_prepare_args(ctxs2, ctxspav_2, ctxspav, 3);

        if (ret < 0) {
            lsdif_mmc_write_args(ctxs2, ctxspav_2);
            goto DRDY_ERR;
        }

        unkcmd0 = ctxspav[0x6e];
        unkcmd1 = ctxspav[0x6f];
        unkcmd2 = ctxspav[0x6c];
        unkcmd3 = ctxspav[0x6d];
        ret = *ctx;
        *(int *)(ctxs2 + 0x2498) = unkcmd0 - unkcmd2;
        *(int *)(ctxs2 + 0x249c) = unkcmd1 - (unkcmd3 + (unsigned int)(unkcmd0 < unkcmd2));
        ret = lsdif_ctrl_apply_cmd(ret, rand);

        if (-1 < ret) {
            lsdif_mmc_write_args(ctxs2, ctxspav_2);
            goto CAOP_END;
        }

        lsdif_mmc_write_args(ctxs2, ctxspav_2);
        lsdif_mmc_write_args(ctxs2, ctxspav);
    } else {
        ret = lsdif_mmc_prepare_args(ctxs2, ctxspav, 0, 3);

        if (-1 < ret) {
            unkcmd0 = ctxspav[0x6e];
            unkcmd1 = ctxspav[0x6f];
            unkcmd2 = ctxspav[0x6c];
            unkcmd3 = ctxspav[0x6d];
            ret = *ctx;
            *(int *)(ctxs2 + 0x2498) = unkcmd0 - unkcmd2;
            *(int *)(ctxs2 + 0x249c) = unkcmd1 - (unkcmd3 + (uint32_t)(unkcmd0 < unkcmd2));
            ret = lsdif_ctrl_apply_cmd(ret, rand);

            if (-1 < ret) {
            CAOP_END:
                lsdif_mmc_write_args(ctxs2, ctxspav);

                if ((*(char *)(ctxs2 + 0x2439) == 1) && (*(int *)(ctxs2 + 0x2420) == 1)) {
                    *(char *)(ctxs2 + 0x243b) = *(char *)(ctxs2 + 0x243b) + 1;
                }

                ret = 0;
                goto EXIT;
            }
        }
    DRDY_ERR:
        lsdif_mmc_write_args(ctxs2, ctxspav);
    }
EXIT:
    return ret;
}

int g_disable_bootarea_update = 1;
int write_sector_mmc(int *ctx, unsigned int block_offset, const void *target_buf, int block_count) {
    conlog("WRITE to MMC, block_offset: %u, block_count: %d\n", block_offset, block_count);
    if (g_disable_bootarea_update && (ctx == (int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX)) {
        if (block_offset < SBLS_END && (block_offset >= SBLS_START || block_offset < IDSTOR_START))
            return -1;
    }
    return write_sector_mmc_direct(ctx, block_offset, target_buf, block_count);
}

int sd_init(int tries, int init_sd0_part) {  // copied from enso_ex core
    if (*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX) {
		LOG("SD slot already likely initialized\n");
		return 0;
	}
    syscon_short_command_write(0x888, 1, 1);                                 // enable the GC slot
    ((struct sysroot_buffer *)ns_kbl_param)->boot_type_indicator_1 |= 0x40000;  // enable sd0 mounting
	nskbl_clean_dcache((void *)ns_kbl_param, 0x100);
	int ret = 0;
	do {
        ret = nskbl_init_sd((unsigned int *)(*(uint32_t *)NSKBL_DEVICE_GCSD_CTX), (int *)NSKBL_DEVICE_GCSD_TGT_CTX);
		if (!ret)
			break;
		LOG("SD init failed: 0x%08X, retrying %d...\n", ret, tries);
    } while (--tries);
	if (!ret)
		LOG("SD slot initialized successfully\n");
	if (init_sd0_part && !ret) {
        ret = nskbl_init_part(((init_sd0_part == 2) ? (unsigned int *)NSKBL_PARTITION_OS0 : (unsigned int *)NSKBL_PARTITION_SD0), 
								0x100000, 
								(unsigned int *)nskbl_switch_read_dev,
                              	(unsigned int *)NSKBL_DEVICE_GCSD_CTX
							);
        if (ret < 0) {
			LOG("Failed to init the GC-SD partition: %d\n", ret);
			return ret;
		}
		LOG("GC-SD partition initialized successfully\n");
	}
	return ret;
}

void reinit_nskbl_storages(int with_sd) {
    if (with_sd > 0) {
        LOG("Enabling GC-SD support..\n");
        syscon_short_command_write(0x888, 1, 1);
        ((struct sysroot_buffer *)ns_kbl_param)->boot_type_indicator_1 |= 0x40000;  // enable sd0 mounting
        nskbl_clean_dcache((void *)ns_kbl_param, 0x100);
        delay(1000);
    } else if (with_sd < 0) {
        LOG("Disabling GC-SD support..\n");
        // syscon_short_command_write(0x888, 0, 1);
        ((struct sysroot_buffer *)ns_kbl_param)->boot_type_indicator_1 &= ~0x40000;  // disable sd0 mounting
        nskbl_clean_dcache((void *)ns_kbl_param, 0x100);
        delay(1000);
    }
    LOG("Reinitializing nskbl storages (without os0 init)\n");
    uint32_t prev = *(volatile uint32_t *)NSKBL_SETUP_EMMC_INIT_OS0_CALL;
    *(volatile uint32_t *)NSKBL_SETUP_EMMC_INIT_OS0_CALL = 0xbf00bf00;  // nop
    nskbl_clean_dcache((void *)NSKBL_SETUP_EMMC_INIT_OS0_CALL_CACHER, 0x20);
    nskbl_flush_icache();
    int ret = nskbl_setup_emmc();
    *(volatile uint32_t *)NSKBL_SETUP_EMMC_INIT_OS0_CALL = prev;  // restore
    nskbl_clean_dcache((void *)NSKBL_SETUP_EMMC_INIT_OS0_CALL_CACHER, 0x20);
    nskbl_flush_icache();
    LOG("setup_emmc 0x%08X\n", ret);
    if (with_sd > 0)
        LOG("SD CTX: 0x%08X\n", *(volatile uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX);
}

const char *get_partition_name(int part) {
    static char *names[] = {
        "invalid",
        "idstor",
        "sloader",
        "os",
        "vsh",
        "vshdata",
        "vtrm",
        "user",
        "userext",
        "gamero",
        "gamerw",
        "updater",
        "sysdata",
        "mediaid",
        "pidata",
        "unused"
    };
    if (part < 0 || part >= sizeof(names) / sizeof(names[0]))
        return "invalid";
    return names[part];
}

static struct mount_master_ctx l_mount_master[2] = {
    [MOUNT_MASTER_EMMC] = {
        .type = MOUNT_MASTER_TYPE_NONE,
        .dev_ctx = NULL,
        .read_sector = NULL,
        .write_sector = NULL,
    },
    [MOUNT_MASTER_GCSD] = {
        .type = MOUNT_MASTER_TYPE_NONE,
        .dev_ctx = NULL,
        .read_sector = NULL,
        .write_sector = NULL,
    }
};

int stor_init_master(int mount_master) {
    int ret = 0;
    // get MBR
    char s0[SECTOR_SIZE];
    switch(mount_master) {
        case MOUNT_MASTER_EMMC:
            ret = MMCREAD(0, s0, sizeof(master_block_t) / SECTOR_SIZE);
            break;
        case MOUNT_MASTER_GCSD:
            if (!IS_GCSD_INITIALIZED()) {
                LOG("GC-SD not initialized, cannot read master block\n");
                return -1;
            }
            ret = SDREAD(0, s0, sizeof(master_block_t) / SECTOR_SIZE);
            break;
        default:
            LOG("Invalid mount master: %d\n", mount_master);
            return -1;
    }
    if (ret < 0) {
        LOG("Failed to read master block for mount master %d: %d\n", mount_master, ret);
        return ret;
    }
    if (((master_block_t *)s0)->sig != FAT_MBR_MAGIC) {
        LOG("Invalid master block signature for mount master %d: 0x%04X\n", mount_master, ((master_block_t *)s0)->sig);
        return -1;
    }

    // we good, invalidate and initialize the mount master
    struct mount_master_ctx *mm = &l_mount_master[mount_master];
    memset(mm, 0, sizeof(struct mount_master_ctx));
    memcpy(&mm->sector0, s0, sizeof(master_block_t));

    // ctx
    switch (mount_master) {
        case MOUNT_MASTER_EMMC:
            mm->dev_ctx = (int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX;
            mm->read_sector = read_sector_mmc_direct;
            mm->write_sector = write_sector_mmc;
            break;
        case MOUNT_MASTER_GCSD:
            mm->dev_ctx = (int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX;
            mm->read_sector = read_sector_sd;
            mm->write_sector = write_sector_sd;
            break;
        default:
            LOG("Invalid mount master: %d\n", mount_master);
            return -1;
    }

    // type
    if ((*(uint32_t *)s0 == SCEMBR_U32_MAGIC))
        mm->type = MOUNT_MASTER_TYPE_SCE;
    else
        mm->type = MOUNT_MASTER_TYPE_FAT;

    LOG("Mount master %d initialized successfully, type: %d\n", mount_master, mm->type);
    return 0;
}

static partition_t *find_partition_by_id(master_block_t *master, int part_id) {
    if (part_id < 1 || part_id >= ARRAYSIZE(master->partitions)) {
        LOG("Invalid partition ID: %d\n", part_id);
        return NULL;
    }
    for (int i = 0; i < ARRAYSIZE(master->partitions); i++) {
        if (master->partitions[i].code == part_id)
            return &master->partitions[i];
    }
    return NULL;
}

static struct mount_ctx mount_dev[STOR_MAX_MOUNTS];

int stor_init_mount(int idx, int mount_master, int partition_id) {
    if (idx < 0 || idx >= STOR_MAX_MOUNTS) {
        LOG("Invalid mount index: %d\n", idx);
        return -1;
    }
    if (mount_master < 0 || mount_master > MOUNT_MASTER_GCSD) {
        LOG("Invalid mount master: %d\n", mount_master);
        return -1;
    }
    if (partition_id < 0 || partition_id > 15) {
        LOG("Invalid partition ID: %d\n", partition_id);
        return -1;
    }
    struct mount_master_ctx *mm = &l_mount_master[mount_master];
    if (mm->type == MOUNT_MASTER_TYPE_NONE) {
        LOG("Mount master %d not initialized\n", mount_master);
        return -1;
    }
    struct mount_ctx *ctx = &mount_dev[idx];
    memset(ctx, 0, sizeof(struct mount_ctx));
    ctx->master = mm;

    if (partition_id) {
        partition_t *part = find_partition_by_id(&ctx->master->sector0, partition_id);
        if (!part) {
            LOG("Partition not found: %d [%s] on mount master %d\n", partition_id, get_partition_name(partition_id), mount_master);
            return -1;
        }
        ctx->params = part;
    } else
        ctx->params = &ctx->master->sector0.partitions[0];  // default to first partition

    LOG("Mount context %d initialized for partition %s on mount master %d\n", idx, get_partition_name(ctx->params->code), mount_master);
    return 0;
}

static struct mount_ctx *get_validate_mctx(int idx) {
    if (idx < 0 || idx >= STOR_MAX_MOUNTS) {
        LOG("Invalid mount index: %d\n", idx);
        return NULL;
    }
    struct mount_ctx *ctx = &mount_dev[idx];
    if (!ctx->master || !ctx->master->type) {
        LOG("Master context not initialized for mount index %d\n", idx);
        return NULL;
    }
    if (!ctx->params) {
        LOG("Partition parameters not set for mount index %d\n", idx);
        return NULL;
    }

    return ctx;
}

static uint32_t get_validate_msector(struct mount_ctx *ctx, int sector, int nsectors) {
    if (ctx->master->type == MOUNT_MASTER_TYPE_SCE) {
        if ((sector < 0 || sector >= ctx->params->sz) || (nsectors <= 0 || nsectors > (ctx->params->sz - sector))) {
            LOG("Invalid sector range: %d, nsectors: %d for partition %s\n", sector, nsectors, get_partition_name(ctx->params->code));
            return 0xFFFFFFFF;  // invalid sector
        }
        sector += ctx->params->off;
    }
    return sector;
}

int stor_read_mount(int idx, uint32_t sector, void *buffer, int nsectors) {
    conlog("READ from mount %d, sector: %u, nsectors: %d\n", idx, sector, nsectors);
    struct mount_ctx *ctx = get_validate_mctx(idx);
    if (!ctx)
        return -1;
    sector = get_validate_msector(ctx, sector, nsectors);
    if (sector == 0xFFFFFFFF)
        return -1;
    return ctx->master->read_sector(ctx->master->dev_ctx, sector, buffer, nsectors);
}

int stor_write_mount(int idx, uint32_t sector, const void *buffer, int nsectors) {
    conlog("WRITE to mount %d, sector: %u, nsectors: %d\n", idx, sector, nsectors);
    struct mount_ctx *ctx = get_validate_mctx(idx);
    if (!ctx)
        return -1;
    sector = get_validate_msector(ctx, sector, nsectors);
    if (sector == 0xFFFFFFFF)
        return -1;
    return ctx->master->write_sector(ctx->master->dev_ctx, sector, buffer, nsectors);
}

int stor_ff_init_mount(int idx) {
    if (!get_validate_mctx(idx)) {
        LOG("(FF) Invalid mount context for index %d\n", idx);
        return -1;
    }
    return 0;
}