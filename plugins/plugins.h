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

typedef struct _kbl_param_s {
  uint16_t version;
  uint16_t size;
  uint32_t sl_ver;
  uint32_t min_sl_ver;
  uint32_t unk_C;
  uint32_t unk_10;
  uint32_t unk_14;
  uint32_t unk_18;
  uint32_t unk_1C;
  struct {
    uint8_t qa[0x10];
    uint8_t nvs[0x10];
    uint8_t dipsw[0x20];
  } flags;
  uint32_t dram_start;
  uint32_t dram_size;
  uint32_t unk_68;
  uint32_t device_mode;
  uint8_t opsid[0x10];
  uint32_t sk_enp_start;
  uint32_t sk_enp_size;
  uint32_t ctxa_sm_start;
  uint32_t ctxa_sm_size;
  uint32_t kprxa_sm_start;
  uint32_t kprxa_sm_size;
  uint32_t srvk_start;
  uint32_t srvk_size;
  uint64_t pscode;
  uint64_t stack_cookie;
  uint8_t session_id[0x10];
  uint32_t wakeup_req;
  uint32_t wakeup_factor;
  uint32_t usb_status;
  uint32_t ctrl;
  uint32_t resume_addr;
  uint32_t hw_cfg;
  uint32_t boot_cause;
  uint32_t unk_DC;
  uint32_t unk_resume;
  uint32_t unk_E4;
  uint8_t hw_cfg_ext[0x10];
  uint32_t sl_rev;
  uint32_t magic;
  uint8_t session_key[0x20];
  uint8_t unused[0xE0];
} __attribute__((packed)) kbl_param_s;

// This struct is passed to custom plugins at module_start
// REF: ex_ports_struct @ /base/ex_defs.h
// REF: prepare_modlists @ /plugins/loader/kernel.c
typedef struct patch_args_struct {
  uint32_t this_version; // version of this struct
  uint32_t ex_ctrl; // ex ctrl data
  void* nskbl_exports_start; // nskbl exports start
  kbl_param_s* kbl_param;
  int (*ex_get_file)(char* file_path, void* buf, uint32_t read_size, uint32_t offset); // e2x's get_file func
  void* (*kbl_memset)(void* dst, int ch, int sz);
  void* (*kbl_memcpy)(void* dst, const void* src, int sz);
  void* (*kbl_get_obj_for_uid)(int uid);
  int (*kbl_alloc_memblock)(const char* name, int type, int size, void* opt);
  int (*kbl_get_memblock)(int32_t uid, void** basep);
  int (*kbl_free_memblock)(int32_t uid);
  int *ex_protect_boot; // pointer to boot area protection flag
  int (*ex_init_os0)(uint32_t mbr_off, unsigned int* ctx, int is_scembr);
  void (*printf)(const char* fmt, ...);
  void* defarg; // default arg passed to all boot modules at start
  int* uids_a; // first uid list
  int* uids_b; // second uid list
  int *uids_d; // devkit uid list
} patch_args_struct;
#define PATCH_ARGS_VERSION 4