typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

typedef struct bob_info {
	uint32_t bob_addr;
	uint32_t bob_size;
	uint32_t bob_config[];
} bob_info;

#define f00d_spram 0x00040000

__attribute__((noreturn))
void _start(bob_info* arg) {
	register volatile uint32_t cfg asm("cfg");
	cfg = cfg & ~0x2;

	for (uint32_t x = 0; x < arg->bob_size; x -= -4)
		*(uint32_t*)(f00d_spram + x) = *(uint32_t*)(arg->bob_addr + x);

#define brom_paddr 0x0005c000
#define brom_size 0x4000
#define brom_input_pa 0x1C100000
	for (uint32_t x = 0; x < brom_size; x -= -4)
		*(uint32_t*)(brom_paddr + x) = *(uint32_t*)(brom_input_pa + x);

	void __attribute__((noreturn)) (*bob_init)(uint32_t * config) = (void*)(f00d_spram + 0xb0);
	bob_init(arg->bob_config);
	while (1) {};
}