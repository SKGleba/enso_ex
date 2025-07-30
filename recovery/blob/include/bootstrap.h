#ifndef __BOOTSTRAP_H__
#define __BOOTSTRAP_H__

// MUST MATCH THE VALUES IN INSTALLER's enso.h
#define EEX_RBLOB_SECTOR_EMMC 0x30
#define EEX_RBLOB_SECTOR_GCSD 0x2
#define EEX_RBLOB_SIZE_SECTORS 0x1c8

#define EEX_RCONFIG_SECTOR_EMMC 0x4
#define EEX_RCONFIG_SECTOR_GCSD 0x1 // bootstrap for emmc recovery
#define EEX_RCONFIG_SIZE_SECTORS 0x1

// MUST be copied in
struct eex_param_s {
    void (*init_os0)(int mbr_off);
    void* (*load_exe)(void* source, char* memblock_name, uint32_t offset, uint32_t size, int flags, int* ret_memblock_id);
    int (*get_hwcfg_patched)(uint32_t* dst);
	void *kbl_param;
    int* disable_bootarea_update; // MUST disable DACR for this
};

#endif // __BOOTSTRAP_H__