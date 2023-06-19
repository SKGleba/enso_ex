/* plugins.h -- data that custom plugins should be aware about
 *
 * Copyright (C) 2018-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#define CTRL_BUTTON_HELD(ctrl, button)		!((ctrl) & (button))
#define CTRL_BUTTON_PRESSED(ctrl, old, button)	!(((ctrl) & ~(old)) & (button))
#define CTRL_UP		(1 << 0)
#define CTRL_RIGHT	(1 << 1)
#define CTRL_DOWN	(1 << 2)
#define CTRL_LEFT	(1 << 3)
#define CTRL_TRIANGLE	(1 << 4)
#define CTRL_CIRCLE	(1 << 5)
#define CTRL_CROSS	(1 << 6)
#define CTRL_SQUARE	(1 << 7)
#define CTRL_SELECT	(1 << 8)
#define CTRL_L		(1 << 9)
#define CTRL_R		(1 << 10)
#define CTRL_START	(1 << 11)
#define CTRL_PSBUTTON	(1 << 12)
#define CTRL_POWER	(1 << 14)
#define CTRL_VOLUP	(1 << 16)
#define CTRL_VOLDOWN	(1 << 17)
#define CTRL_HEADPHONE	(1 << 27)

typedef struct kbl_param_struct {
  uint16_t version;
  uint16_t size;
  uint32_t fw_version;
  uint32_t ship_version;
  uint32_t field_C;
  uint32_t field_10;
  uint32_t field_14;
  uint32_t field_18;
  uint32_t field_1C;
  uint32_t field_20;
  uint32_t field_24;
  uint32_t field_28;
  uint8_t debug_flags[8];
  uint32_t field_34;
  uint32_t field_38;
  uint32_t field_3C;
  uint8_t dip_switches[0x20];
  uint32_t dram_base;
  uint32_t dram_size;
  uint32_t field_68;
  uint32_t boot_type_indicator_1;
  uint8_t serial[0x10];
  uint32_t secure_kernel_enp_addr;
  uint32_t secure_kernel_enp_size;
  uint32_t field_88;
  uint32_t field_8C;
  uint32_t kprx_auth_sm_self_addr;
  uint32_t kprx_auth_sm_self_size;
  uint32_t prog_rvk_srvk_addr;
  uint32_t prog_rvk_srvk_size;
  uint16_t model;
  uint16_t device_type;
  uint16_t device_config;
  uint16_t retail_type;
  uint32_t field_A8;
  uint32_t field_AC;
  uint8_t session_id[0x10];
  uint32_t field_C0;
  uint32_t boot_type_indicator_2;
  uint32_t field_C8;
  uint32_t field_CC;
  uint32_t resume_context_addr;
  uint32_t field_D4;
  uint32_t field_D8;
  uint32_t field_DC;
  uint32_t field_E0;
  uint32_t field_E4;
  uint32_t field_E8;
  uint32_t field_EC;
  uint32_t field_F0;
  uint32_t field_F4;
  uint32_t bootldr_revision;
  uint32_t magic;
  uint8_t session_key[0x20];
  uint8_t unused[0xE0];
} __attribute__((packed)) kbl_param_struct;

// This struct is passed to custom plugins at module_start
// REF: ex_ports_struct @ /base/ex_defs.h
// REF: prepare_modlists @ /plugins/loader/kernel.c
typedef struct patch_args_struct {
  uint32_t this_version; // version of this struct
  uint32_t ex_ctrl; // ex ctrl data
  void* nskbl_exports_start; // nskbl exports start
  kbl_param_struct* kbl_param;
  void* (*ex_load_exe)(void* source, char* memblock_name, uint32_t offset, uint32_t size, int flags, int* ret_memblock_id); // e2x's load_exe func
  int (*ex_get_file)(char* file_path, void* buf, uint32_t read_size, uint32_t offset); // e2x's get_file func
  void* (*kbl_memset)(void* dst, int ch, int sz);
  void* (*kbl_memcpy)(void* dst, const void* src, int sz);
  void* (*kbl_get_obj_for_uid)(int uid);
  int (*kbl_alloc_memblock)(const char* name, int type, int size, void* opt);
  int (*kbl_get_memblock)(int32_t uid, void** basep);
  int (*kbl_free_memblock)(int32_t uid);
  void* defarg; // default arg passed to all boot modules at start
  int* uids_a; // first uid list
  int* uids_b; // second uid list
  int *uids_d; // devkit uid list
} __attribute__((packed)) patch_args_struct;
#define PATCH_ARGS_VERSION 3