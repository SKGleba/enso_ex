#include "utils.h"
#include "paper.h"
#include "nskbl.h"

int g_log_targets = LOG_TARGET_NONE;
void dbg_log(int targets, const char *fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
	if (targets & LOG_TARGET_CONSOLE)
		nskbl_printf("[R] %s", buffer);
	if (targets & LOG_TARGET_PAPER)
        paper_write(&default_paper, buffer, sizeof(buffer));
}

void dbg_hexdump(void *addr, int size, bool show_addr, char delim) {
	unsigned char *ptr = (unsigned char *)addr;
	int i, j;
	char line[80];
	for (i = 0; i < size; i += 16) {
		if (show_addr)
			snprintf(line, sizeof(line), "%08X: ", i);
		else
			line[0] = '\0';

		for (j = 0; j < 16 && (i + j) < size; j++) {
			snprintf(line + strlen(line), sizeof(line) - strlen(line), "%02X%c", ptr[i + j], delim);
		}

		LOG("%s\n", line);
	}
}