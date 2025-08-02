#include "utils.h"
#include "nskbl.h"
#include "main.h"

// libcx
char *my_strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c)
            return (char *)s;
        s++;
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