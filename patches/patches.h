#define E2X_EPATCHES_SKIP CTRL_VOLUP
#define E2X_USE_BBCONFIG CTRL_SQUARE
#define E2X_RECOVERY_RUNDN CTRL_START

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
typedef struct patch_args_struct {
  void *defarg; // default arg passed to all boot modules at start
  kbl_param_struct* kbl_param;
  uint32_t *nskbl_exports; // nskbl exports start
  uint32_t ctrldata; // current ctrl data
  void* load_exe; // e2x's load_exe func
  void* get_file; // e2x's get_file func
  int* uids_a; // first uid list
  int *uids_b; // second uid list
  int *uids_d; // devkit uid list
} __attribute__((packed)) patch_args_struct;

// 3.65 nskbl funcs
static void *(*kbl_memset)(void *dst, int ch, int sz) = (void*)0x51013C41;
static void *(*kbl_memcpy)(void *dst, const void *src, int sz) = (void *)0x51013BC1;
static void *(*get_obj_for_uid)(int uid) = (void *)0x51017785;
static int (*sceKernelAllocMemBlock)(const char *name, int type, int size, void *opt) = (void *)0x51007161;
static int (*sceKernelGetMemBlockBase)(int32_t uid, void **basep) = (void *)0x510057E1;
static int (*sceKernelFreeMemBlock)(int32_t uid) = (void *)0x51007449;