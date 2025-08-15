#include "utils.h"
#include "nskbl.h"
#include "main.h"
#include "stor.h"
#include "fmgr.h"

char *my_strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    return NULL;
}

char *my_strrchr(const char *s, int c) {
	const char *last = NULL;
	while (*s) {
		if (*s == (char)c)
			last = s;
		s++;
	}
	return (char *)last;
}

char *find_nth(const char *s, char c, int n) {
	const char *ptr = s;
	while (*ptr && n > 0) {
		if (*ptr == c)
			n--;
		ptr++;
	}
	return (n == 0) ? (char *)(ptr - 1) : NULL;  // return the last found character or NULL if not found
}

int count_chs(const char *s, char c) {
	int count = 0;
	while (*s) {
		if (*s == c)
			count++;
		s++;
	}
	return count;
}

char *find_rnth(const char *s, char c, int n) {
	int count = count_chs(s, c);
	if (n > count)
		return NULL;  // if n is greater than the number of occurrences, return NULL
	return find_nth(s, c, count - n + 1);  // find the nth occurrence from the end
}

int antoh(char *input, uint8_t *output, int output_len) {
    for (int i = 0; i < (output_len * 2); i++) {
        if (input[i] < '0' || (input[i] > '9' && input[i] < 'A') || input[i] > 'F')
            return -1;
    }

    for (int i = 0; i < output_len; i++) {
        if (input[i * 2] < 'A')
            output[i] = 0x10 * (input[i * 2] - '0');
        else
            output[i] = 0x10 * (input[i * 2] - '7');

        if (input[(i * 2) + 1] < 0x40)
            output[i] += (input[(i * 2) + 1] - '0');
        else
            output[i] += (input[(i * 2) + 1] - '7');
    }

    return 0;
}

static const char hexbase[] = "0123456789ABCDEF";
int hntoa(uint8_t *input, char *output, int output_len) {
    if (output_len & 1)
        return -1;

    output_len = output_len / 2;

    for (int i = 0; i < output_len; i -= -1) {
        output[i * 2] = hexbase[(input[i] & 0xF0) >> 4];
        output[(i * 2) + 1] = hexbase[input[i] & 0x0F];
    }

    return 0;
}

char *find_endline(char *start, char *end) {
    for (char *ret = start; ret < end; ret++) {
        if (*(uint16_t *)ret == 0x0A0D || *(uint8_t *)ret == 0x0A)
            return ret;
    }
    return end;
}

char *find_nextline(char *current_line_end, char *end) {
    for (char *next_line = current_line_end; next_line < end; next_line++) {
        if (*(uint8_t *)next_line != 0x0D && *(uint8_t *)next_line != 0x0A && *(uint8_t *)next_line != 0x00)
            return next_line;
    }
    return NULL;
}

int g_log_targets = LOG_TARGET_NONE;
void dbg_log(int targets, const char *fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
	if (targets & LOG_TARGET_CONSOLE)
		nskbl_printf("[R] %s", buffer);
	if (targets & LOG_TARGET_LOGPAPER)
        paper_write(&default_paper, buffer, sizeof(buffer));
	if (targets & LOG_TARGET_FRONTPAGE)
		paper_write(&info_paper, buffer, sizeof(buffer));
}

void dbg_hexdump(void *addr, int size, uint32_t show_addr, char delim) {
	unsigned char *ptr = (unsigned char *)addr;
	int i, j;
	char line[80];
	for (i = 0; i < size; i += 16) {
		if (show_addr != 0xFFFFFFFF)
			my_snprintf(line, sizeof(line), "%08X: ", i + show_addr);
		else
			line[0] = '\0';

		for (j = 0; j < 16 && (i + j) < size; j++) {
			my_snprintf(line + strlen(line), sizeof(line) - strlen(line), "%02X%c", ptr[i + j], delim);
		}

		LOG("%s\n", line);
	}
}

struct rmemblock_master_s rmemblock_master;
static void *rmemblock_palloc(int size, uint32_t opt_type, uint32_t opt_paddr) {
	LOG("PAllocating rmemblock: size=%d, type=0x%08X, paddr=0x%08X\n", size, opt_type, opt_paddr);
    char name[8];
    struct rmemblock_info_s *rblock = NULL;
    int start = (!size) ? 0 : RMEMBLOCK_BB_COUNT; // for BB blocks
	int end = (!size) ? RMEMBLOCK_BB_COUNT : RMEMBLOCK_PA_COUNT; // ^
    for (int i = start; i < end; i++) {
        if (rmemblock_master.pallocs[i].id < 0) {
            rblock = &rmemblock_master.pallocs[i];
			//LOG("Found free rmemblock slot at index %d\n", i);
			my_snprintf(name, sizeof(name), "rblk%d", i);
			break;
        }
    }
    if (!rblock) {
		LOG("ERROR: No free rmemblock slots available\n");
		return NULL;
	}
	if (!size)
		size = RMEMBLOCK_MIN_SIZE;
	size = (size + (RMEMBLOCK_MIN_SIZE - 1)) & ~(RMEMBLOCK_MIN_SIZE - 1);  // align to 4KB
	if (opt_paddr) {
        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(opt));
        opt.size = sizeof(opt);
        opt.paddr = opt_paddr;
        opt.attr = 2;
        rblock->id = sceKernelAllocMemBlock(name, opt_type ?: MEMBLOCK_TYPE_RW, size, &opt);
    } else
		rblock->id = sceKernelAllocMemBlock(name, opt_type ?: MEMBLOCK_TYPE_RW, size, NULL);
	if (rblock->id < 0) {
		LOG("ERROR: Failed to allocate memory block %s (size=%d, type=0x%08X): %08X\n", name, size, opt_type, rblock->id);
		return NULL;
	}
	sceKernelGetMemBlockBase(rblock->id, &rblock->va);
	if (!rblock->va) {
		LOG("ERROR: Failed to get memory block base for %s (id=%d)\n", name, rblock->id);
		sceKernelFreeMemBlock(rblock->id);
		rblock->id = -1;
		return NULL;
	}
    //LOG("INFO: Memory block %s (id=%d) allocated successfully @ 0x%08X\n", name, rblock->id, (unsigned int)rblock->va);
    return rblock->va;
}

void *rmemblock_alloc(int size, uint32_t opt_type, uint32_t opt_paddr) {
	if (opt_paddr)
        return rmemblock_palloc(size, opt_type, (opt_paddr & 0xFFFFF000) ? opt_paddr : 0);
    if (!opt_type && (size <= RMEMBLOCK_SMB_SIZE)) {
		for (int x = 0; x < RMEMBLOCK_BB_COUNT; x++) {
			if (rmemblock_master.pallocs[x].id < 0) {
				LOG("Allocating big block %d\n", x);
                if (!rmemblock_palloc(0, MEMBLOCK_TYPE_RW, 0))
                    goto def_alloc;
            }
			for (int i = 0; i < 16; i++) {
				if (rmemblock_master.smbe[x] & BITN(i)) {
					LOG("Using free small block %d in big block %d\n", i, x);
					rmemblock_master.smbe[x] &= ~BITN(i);
					return rmemblock_master.pallocs[x].va + (i * RMEMBLOCK_SMB_SIZE);
				}
			}
		}
		LOG("Ran out of small blocks, allocating normally\n");
	}
def_alloc:
    size = ((size + sizeof(struct rmemblock_info_s)) + (RMEMBLOCK_MIN_SIZE - 1)) & ~(RMEMBLOCK_MIN_SIZE - 1);  // align to 4KB
    LOG("Allocating rmemblock: size=%d, type=0x%08X\n", size, opt_type);
    int block_id = sceKernelAllocMemBlock("e2xr_malloc", opt_type ?: MEMBLOCK_TYPE_RW, size, NULL);
	if (block_id < 0) {
		LOG("ERROR: Failed to allocate memory block (size=%d, type=0x%08X): %08X\n", size, opt_type, block_id);
		return NULL;
	}
	void *block_va = NULL;
	sceKernelGetMemBlockBase(block_id, &block_va);
	if ((sceKernelGetMemBlockBase(block_id, &block_va) < 0) || !block_va) {
		LOG("ERROR: Failed to get memory block base for 0x%08X\n", block_id);
		sceKernelFreeMemBlock(block_id);
		return NULL;
	}
	//LOG("INFO: Memory block 0x%08X allocated successfully @ 0x%08X [user=0x%08X]\n", block_id, (unsigned int)block_va, (unsigned int)block_va + sizeof(struct rmemblock_info_s));
    ((struct rmemblock_info_s *)block_va)->id = block_id;
    ((struct rmemblock_info_s *)block_va)->va = block_va;
    return block_va + sizeof(struct rmemblock_info_s);
}

int rmemblock_remap(void *va, uint32_t type) {
    int ret = 0;
    const char *op = type ? "remap" : "free";
    LOG("Try %s rmemblock at VA: 0x%08X\n", op, (unsigned int)va);
    for (int i = 0; i < RMEMBLOCK_PA_COUNT; i++) {
        if (i >= RMEMBLOCK_BB_COUNT && (rmemblock_master.pallocs[i].va == va)) {
            int id = rmemblock_master.pallocs[i].id;
			if (id < 0) {
				LOG("ERROR: Attempted to %s an unallocated memory block at 0x%08X\n", op, (unsigned int)va);
				return -1;
			}
			if (!type) {
				rmemblock_master.pallocs[i].id = -1;
				rmemblock_master.pallocs[i].va = NULL;
				ret = sceKernelFreeMemBlock(id);
			} else
				ret = sceKernelRemapBlock(id, type);
			if (ret < 0) {
				LOG("ERROR: Failed to %s memory block %d: %08X\n", op, id, ret);
				return ret;
			}
			//LOG("INFO: Memory block %d %sd successfully\n", id, op);
			return 0;
        } else if (i < RMEMBLOCK_BB_COUNT && (va >= rmemblock_master.pallocs[i].va) && (va < (rmemblock_master.pallocs[i].va + RMEMBLOCK_MIN_SIZE))) {
			if (type) {
				LOG("ERROR: Cannot remap small blocks, only free them\n");
				return -1;
			}
            int j = (va - rmemblock_master.pallocs[i].va) / RMEMBLOCK_SMB_SIZE;
			if (j < 16 && !(rmemblock_master.smbe[i] & BITN(j))) {
				rmemblock_master.smbe[i] |= BITN(j);
				//LOG("Freed small block %d in big block %d\n", j, i);
				return 0;
			} else {
				LOG("ERROR: Invalid small block index %d for VA: 0x%08X\n", j, (unsigned int)va);
				return -1;
			}
        }
    }
    //LOG("INFO: treating VA: 0x%08X as mallocd memblock, trying -%d for info\n", (unsigned int)va, sizeof(struct rmemblock_info_s));
	struct rmemblock_info_s * actualva = (struct rmemblock_info_s *)(va - sizeof(struct rmemblock_info_s));
	if (actualva->va != (void*)actualva) {
		LOG("ERROR: Attempted to %s an invalid rmemblock at 0x%08X\n", op, (unsigned int)va);
		return -1;
	}
	if (type)
        ret = sceKernelRemapBlock(actualva->id, type);
    else
        ret = sceKernelFreeMemBlock(actualva->id);
    if (ret < 0) {
        LOG("ERROR: Failed to %s memory block 0x%08X: %08X\n", op, actualva->id, ret);
        return ret;
	}
	//LOG("INFO: Memory block 0x%08X %sd successfully\n", id, op);
	return 0;
}

void rmemblock_stop(void) {
    for (int i = 0; i < RMEMBLOCK_BB_COUNT; i++) {
        if (rmemblock_master.pallocs[i].id >= 0) {
			LOG("Freeing big block %d\n", i);
            sceKernelFreeMemBlock(rmemblock_master.pallocs[i].id);
            rmemblock_master.pallocs[i].id = -1;
            rmemblock_master.pallocs[i].va = NULL;
        }
    }
	for (int i = RMEMBLOCK_BB_COUNT; i < RMEMBLOCK_PA_COUNT; i++) {
		if (rmemblock_master.pallocs[i].id >= 0)
			LOG("WARNING: Leftover PAllocation %d: 0x%08X @ 0x%08X\n", i, (unsigned int)rmemblock_master.pallocs[i].id, (unsigned int)rmemblock_master.pallocs[i].va);
	}
}

static struct idstorage_s {
	int initialized;
	struct {
		void *base;
		int count;
	} master;
	struct {
		uint32_t off;
		uint32_t sz;
	} part;
} idstorage_ctx = {
	.initialized = 0,
	.master = {NULL, 0},
	.part = {0, 0}
};
int idstorage_init(void) {
	int ret = 0;
    if (!idstorage_ctx.initialized) {
		uint8_t buf[SECTOR_SIZE];
        master_block_t *mbr = (master_block_t *)buf;
        ret = EMMCREAD(0, mbr, 1);
        if (ret < 0) {
            LOG("ERROR: Failed to read eMMC master block: %08X\n", ret);
            return ret;
        }
        partition_t *idstor = stor_find_partition_by_id(mbr, STOR_PART_IDSTOR, STOR_PART_ACTIVE_BOTH);
        if (!idstor) {
            LOG("ERROR: ID Storage partition not found\n");
            return -1;
        }
		idstorage_ctx.part.off = idstor->off;
		idstorage_ctx.part.sz = idstor->sz;
		if (!idstorage_ctx.part.off || !idstorage_ctx.part.sz) {
			LOG("ERROR: Invalid ID Storage partition off/sz\n");
			return -1;
		}
		if (idstorage_ctx.part.off < IDSTOR_START) {
			LOG("ERROR: ID Storage partition offset is below hardcoded start\n");
			return -1;
		}
		ret = EMMCREAD(idstorage_ctx.part.off, buf, 1);
		if (ret < 0) {
			LOG("ERROR: Failed to read ID Storage master block: %08X\n", ret);
			return ret;
		}
        for (int i = 0; i < (SECTOR_SIZE / sizeof(uint16_t)); i++) {
            if (*(uint16_t *)((uint16_t *)buf + i) == 0xFFF5)
                idstorage_ctx.master.count++;
            else
                break;
        }
        if (!idstorage_ctx.master.count) {
            LOG("ERROR: Could not determine the number of master blocks\n");
            return -1;
        }
        idstorage_ctx.master.base = rmemblock_alloc(idstorage_ctx.master.count * SECTOR_SIZE, MEMBLOCK_TYPE_RW, 0);
        if (!idstorage_ctx.master.base) {
			LOG("ERROR: Failed to allocate memory for ID Storage master blocks\n");
			return -1;
		}
    }
    idstorage_ctx.initialized = 0;
    ret = EMMCREAD(idstorage_ctx.part.off, idstorage_ctx.master.base, idstorage_ctx.master.count);
    if (ret < 0) {
		LOG("ERROR: Failed to read ID Storage master blocks: %08X\n", ret);
		return ret;
	}
    idstorage_ctx.initialized = 1;
    LOG("ID Storage initialized successfully, %d master blocks found\n", idstorage_ctx.master.count);
	return 0;
}

int idstorage_stop(void) {
	idstorage_ctx.initialized = 0;
	rmemblock_free(idstorage_ctx.master.base);
	memset(&idstorage_ctx, 0, sizeof(idstorage_ctx));
	LOG("ID Storage stopped, memory freed\n");
	return 0;
}

int idstorage_rw_leaf(bool write, uint16_t leaf, void *buf) {
	LOG("ID Storage %s leaf: %d\n", write ? "write" : "read", leaf);
	if (!idstorage_ctx.initialized) {
		LOG("ERROR: ID Storage not initialized\n");
		return -1;
	}
	if (leaf < 0 || leaf >= (idstorage_ctx.master.count * ((SECTOR_SIZE / sizeof(uint16_t)) - 1))) {
		LOG("ERROR: Invalid ID Storage leaf index: %d\n", leaf);
		return -1;
	}
	for (int l = 0; l < (idstorage_ctx.master.count * (SECTOR_SIZE / sizeof(uint16_t))); l++) {
		if (*(uint16_t *)((uint16_t *)idstorage_ctx.master.base + l) == leaf) {
			if (write)
				return EMMCWRITE(idstorage_ctx.part.off + l, buf, 1);
			else
				return EMMCREAD(idstorage_ctx.part.off + l, buf, 1);
		}
	}
	LOG("ERROR: ID Storage leaf not found: %d\n", leaf);
	return -1;
}


static uint32_t crc32_tab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b,
    0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0,
    0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd,
    0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
    0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea,
    0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525,
    0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
    0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27,
    0x7d079eb1, 0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda,
    0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703,
    0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
    0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff,
    0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729,
    0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

uint32_t crc32(uint32_t crc, const void *buf, size_t size) {
    const uint8_t *p;

    p = buf;
    crc = crc ^ ~0U;

    while (size--)
        crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

    return crc ^ ~0U;
}


static int txtcfg_getCmdIDX(struct txtcfg_s* cfg, char *line, char *end) {
    int cmd_len = 0;
    int max_cmd_len = end - line;
    for (int i = 1; i < cfg->args.count; i++) {
        cmd_len = strlen(cfg->args.names[i]);
        if (cmd_len < max_cmd_len) {
            if (!memcmp(line, cfg->args.names[i], cmd_len))
                return i;
        }
    }
    return 0;
}

static void txtcfg_prepCmdByIDX(struct txtcfg_s *cfg, int idx, char *command_line, char *end) {
    char *arg = command_line + strlen(cfg->args.names[idx]);
    if (*(uint8_t *)arg != 0x3D)
        return;
    arg++;

    int arglen = end - arg;

    // cut invalid and comments (" " and "#")
    for (int i = 0; i < arglen; i++) {
        if (*(uint8_t *)(arg + i) == 0x20 || *(uint8_t *)(arg + i) == 0x23) {
            arglen = i;
            break;
        }
    }

    if (!arglen || arglen < cfg->args.parsed[idx].min_ascii_arg_len || arglen > cfg->args.parsed[idx].max_ascii_arg_len) {
        return;
	}
	
	if (cfg->args.parsed[idx].ascii_arg)
		return;

    cfg->args.parsed[idx].ascii_arg = my_malloc(arglen + 1);
    if (cfg->args.parsed[idx].ascii_arg) {
        cfg->args.parsed[idx].ascii_arg[arglen] = 0;
        memcpy(cfg->args.parsed[idx].ascii_arg, arg, arglen);
        if (!memcmp(cfg->args.parsed[idx].ascii_arg, arg, arglen)) {
			if (cfg->args.parsed[idx].exec && cfg->args.parsed[idx].cmd_handler) {
                cfg->args.parsed[idx].cmd_handler(idx, cfg->args.parsed[idx].ascii_arg);
                my_free(cfg->args.parsed[idx].ascii_arg);
				cfg->args.parsed[idx].ascii_arg = NULL;
			}
		}
    }
}

void txtcfg_parse(struct txtcfg_s *cfg) {
    char *startconfig = cfg->buf.va;
    char *endconfig = startconfig + cfg->buf.size;

    char *current_line = startconfig;
    char *end_line = startconfig;
    int command_idx = 0;
    while (current_line < endconfig) {
        end_line = find_endline(current_line, endconfig);
        command_idx = txtcfg_getCmdIDX(cfg, current_line, end_line);
        if (command_idx)
            txtcfg_prepCmdByIDX(cfg, command_idx, current_line, end_line);
        current_line = find_nextline(end_line, endconfig);
        if (!current_line)
            break;
    }
}

void txtcfg_cleanup(struct txtcfg_s *cfg) {
    for (int i = 0; i < cfg->args.count; i++) {
        if (cfg->args.parsed[i].ascii_arg) {
            my_free(cfg->args.parsed[i].ascii_arg);
            cfg->args.parsed[i].ascii_arg = NULL;
        }
    }
}

int txtcfg_loadExec(struct txtcfg_s *cfg, bool cleanup) {
	LOG("Loading config file: %s\n", cfg->args.names[0]);
	cfg->buf.va = fmgr_get_file(cfg->args.names[0], NULL, 0, 0, &cfg->buf.size);
	if (!cfg->buf.va)
		return -1;
	txtcfg_parse(cfg);
	LOG("Executing config file: %s\n", cfg->args.names[0]);
	for (int i = 0; i < cfg->args.count; i++) {
		if (cfg->args.parsed[i].ascii_arg && cfg->args.parsed[i].cmd_handler)
			cfg->args.parsed[i].cmd_handler(i, cfg->args.parsed[i].ascii_arg);
	}
	if (cleanup) {
        LOG("Freeing config file: %s\n", cfg->args.names[0]);
        txtcfg_cleanup(cfg);
		my_free(cfg->buf.va);
		cfg->buf.va = NULL;
		cfg->buf.size = 0;
	}
	return 0;
}

int armp_run(struct armp_arg_s *armp, enum CHAIN_FREE_TYPES free) {
    if (armp->magic != ARMP_ARG_MAGIC)
		return -1;
    int i = 0;
	struct armp_d_s *pd = NULL;
    struct armp_d_s *d = (struct armp_d_s *)armp->d;
	while (d) {
        LOG("Processing ARMP D argument %d: %d @ 0x%08X -> 0x%08X\n", i++, d->sz, d->src, d->dst);
		bool fable = d->src_fa;
        DACR_OFF(d->ret = (int)memcpy(d->dst, d->src, d->sz););
        if (fable && d->src && (free & CHAIN_FREE_NESTED))
            my_free(d->src);
		pd = d;
		d = d->next;
		if (free & CHAIN_FREE_ENTRIES)
			my_free(pd);
	}
	i = 0;
	int ret = 0;
	struct armp_x_s *px = NULL;
	struct armp_x_s *x = (struct armp_x_s *)armp->x;
	while (x) {
		LOG("Processing ARMP X argument %d: [0x%08X]0x%08X(0x%08X)\n", i++, x->c_sz, x->src, x->arg);
		if (x->c_sz) {
            void *fbuf = my_malloc(x->c_sz);
            if (fbuf) {
                memcpy(fbuf, (void*)((uint32_t)x->src & ~1), x->c_sz);
				if (my_rxmap(fbuf) >= 0) {
					int (*func)(uint32_t arg) = (int (*)(uint32_t arg))fbuf;
                    if ((uint32_t)x->src & 1)
                        func = (int (*)(uint32_t arg))((uint32_t)fbuf | 1);
					x->ret = func(x->arg);
					LOG("Function returned: 0x%08X\n", x->ret);
				} else {
					LOG("Failed to rxmap function\n");
					x->ret = -1;
					ret = -1;
				}
				if (x->ret & E2X_EXE_RET_NORESIDENT)
					my_free(fbuf);
            } else {
				LOG("Failed to allocate memory for function\n");
                x->ret = -1;
				ret = -1;
            }
			if (x->src && (free & CHAIN_FREE_NESTED))
				my_free(x->src);
        } else {
			x->ret = x->func(x->arg);
			LOG("Function returned: 0x%08X\n", x->ret);
		}
		px = x;
		x = x->next;
		if (free & CHAIN_FREE_ENTRIES)
			my_free(px);
	}
	return ret;
}