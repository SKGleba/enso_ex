// enso_ex defs & structs
#define E2X_MAGIC 0xCAFEBABE
#define E2X_MAX_EPATCHES_N 15
#define E2X_RECOVERY_BLKOFF 0
#define E2X_RECOVERY_FNAME "recovery.e2xp"
#define E2X_RECOVERY_RUNDF CTRL_SELECT
#define E2X_RECOVERY_RUNDN CTRL_START
#define E2X_RECOVERY_NOENT CTRL_TRIANGLE
#define E2X_RECOVERY_SDERR CTRL_CIRCLE
#define E2X_RECOVERY_OSERR CTRL_SQUARE
#define E2X_RECOVERY_RETERR CTRL_CROSS
#define E2X_IPATCHES_SKIP CTRL_VOLDOWN
#define E2X_IPATCHES_FNAME "patches.e2xd"
#define E2X_EPATCHES_UXCFG CTRL_VOLUP

typedef struct RecoveryBlockStruct {
  uint32_t magic;
  uint8_t flags[4];
  uint32_t blkoff;
  uint32_t blksz;
  char data[0x200 - 0x10];
} __attribute__((packed)) RecoveryBlockStruct;

typedef struct PayloadArgsStruct {
  const void *list;
  int *uids;
  int count;
  int safemode;
  unsigned int ctrldata;
  int trun;
} __attribute__((packed)) PayloadArgsStruct;

typedef struct PayloadsBlockStruct {
  uint32_t magic;
  uint32_t count;
  uint32_t off[E2X_MAX_EPATCHES_N];
  uint32_t sz[E2X_MAX_EPATCHES_N];
} __attribute__((packed)) PayloadsBlockStruct;