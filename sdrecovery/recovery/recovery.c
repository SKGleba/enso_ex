// Recovery payload goes here
#include <inttypes.h>
int _start(void *kbl_param, unsigned int ctrldata) {
	return (*(uint32_t *)(kbl_param + 4) == 0x03650000) ? 0 : 4; // continue boot if good fw or let main know if old fw
}