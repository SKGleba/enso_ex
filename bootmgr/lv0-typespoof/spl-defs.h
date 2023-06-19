typedef unsigned char u8_t;             ///< Unsigned 8-bit type
typedef unsigned short int u16_t;       ///< Unsigned 16-bit type
typedef unsigned int u32_t;             ///< Unsigned 32-bit type
typedef unsigned long long u64_t;        ///< Unsigned 64-bit type
typedef unsigned size_t;

typedef struct {
  u16_t magic;
  u8_t unused;
  u8_t status;
  u32_t codepaddr;
  u32_t arg;
  u32_t resp;
} __attribute__((packed)) fm_nfo;