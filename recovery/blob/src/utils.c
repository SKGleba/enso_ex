#include "utils.h"
#include "nskbl.h"
#include "main.h"

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

void dbg_hexdump(void *addr, int size, bool show_addr, char delim) {
	unsigned char *ptr = (unsigned char *)addr;
	int i, j;
	char line[80];
	for (i = 0; i < size; i += 16) {
		if (show_addr)
			my_snprintf(line, sizeof(line), "%08X: ", i);
		else
			line[0] = '\0';

		for (j = 0; j < 16 && (i + j) < size; j++) {
			my_snprintf(line + strlen(line), sizeof(line) - strlen(line), "%02X%c", ptr[i + j], delim);
		}

		LOG("%s\n", line);
	}
}

struct rmemblock_s rmemblock_allocs[RMEMBLOCK_MAX_COUNT];

void *rmemblock_alloc(int size, uint32_t opt_type, uint32_t opt_paddr) {
	LOG("Allocating rmemblock: size=%d, type=0x%08X, paddr=0x%08X\n", size, opt_type, opt_paddr);
    struct rmemblock_s *rblock = NULL;
	for (int i = 0; i < RMEMBLOCK_MAX_COUNT; i++) {
		if (rmemblock_allocs[i].id < 0) {
			rblock = &rmemblock_allocs[i];
			break;
		}
	}
	if (!rblock) {
		LOG("ERROR: No free rmemblock slots available\n");
		return NULL;
	}
	size = (size + (RMEMBLOCK_MIN_SIZE - 1)) & ~(RMEMBLOCK_MIN_SIZE - 1);  // align to 4KB
    char name[8];
	my_snprintf(name, sizeof(name), "rblk%d", (rblock - rmemblock_allocs) / sizeof(struct rmemblock_s));
	if (opt_paddr) {
        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(opt));
        opt.size = sizeof(opt);
        opt.paddr = opt_paddr;
        opt.attr = 0x58;
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
    LOG("INFO: Memory block %s (id=%d) allocated successfully @ 0x%08X\n", name, rblock->id, (unsigned int)rblock->va);
    return rblock->va;
}

int rmemblock_free(void *va) {
	LOG("Freeing rmemblock at VA: 0x%08X\n", (unsigned int)va);
	for (int i = 0; i < RMEMBLOCK_MAX_COUNT; i++) {
		if (rmemblock_allocs[i].va == va) {
			int id = rmemblock_allocs[i].id;
			if (id < 0) {
				LOG("ERROR: Attempted to free an unallocated memory block at 0x%08X\n", (unsigned int)va);
				return -1;
			}
			rmemblock_allocs[i].id = -1;
			rmemblock_allocs[i].va = NULL;
			int ret = sceKernelFreeMemBlock(id);
			if (ret < 0) {
				LOG("ERROR: Failed to free memory block %d: %08X\n", id, ret);
				return ret;
			}
			LOG("INFO: Memory block %d freed successfully\n", id);
			return 0;
		}
	}
	LOG("ERROR: Memory block not found for VA: %p\n", va);
	return -1;
}

int rmemblock_remap(void *va, uint32_t type) {
	LOG("Remapping rmemblock at VA: 0x%08X to type: %d\n", (unsigned int)va, type);
	for (int i = 0; i < RMEMBLOCK_MAX_COUNT; i++) {
		if (rmemblock_allocs[i].va == va) {
			int id = rmemblock_allocs[i].id;
            if (id < 0) {
                LOG("ERROR: Attempted to remap an unallocated memory block at 0x%08X\n", (unsigned int)va);
				return -1;
            }
            int ret = sceKernelRemapBlock(id, type);
			if (ret < 0) {
				LOG("ERROR: Failed to remap memory block %d: %08X\n", id, ret);
				return ret;
			}
			LOG("INFO: Memory block %d remapped successfully\n", id);
			return 0;
		}
	}
	LOG("ERROR: Memory block not found for VA: 0x%08X\n", (unsigned int)va);
	return -1;
}