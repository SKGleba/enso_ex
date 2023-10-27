static const unsigned char bob_loader_nmp[] = {
  0xc0, 0x6f, 0x1a, 0x7b, 0x00, 0x53, 0x06, 0x4b, 0x1e, 0xc0, 0x04, 0x00,
  0x03, 0x03, 0x27, 0xa0, 0x21, 0xc2, 0x10, 0x1c, 0x01, 0xc3, 0xff, 0x0f,
  0x09, 0xe3, 0x08, 0x00, 0x2e, 0x09, 0x21, 0xca, 0xf5, 0xe3, 0xa4, 0xca,
  0x00, 0xc0, 0xa0, 0x92, 0x0a, 0x09, 0x10, 0x62, 0x20, 0x61, 0xb0, 0xd3,
  0x00, 0x04, 0x3f, 0x10, 0x1e, 0x02, 0x21, 0xc9, 0x04, 0x00, 0x32, 0x92,
  0x2e, 0x00, 0x92, 0x93, 0x10, 0x63, 0x2a, 0x00, 0xc4, 0xbf, 0x00, 0x00,
  0xf0, 0xff, 0x1f, 0x00
};

typedef struct bob_config {
	uint32_t ce_framework_parms_addr[2];
	uint32_t uart_params; // (bus << 0x18) | clk
	int run_tests;
} bob_config;

typedef struct bob_info {
	uint32_t bob_addr;
	uint32_t bob_size;
	bob_config config;
} bob_info;

void bob_loader(void) {
	printf("[BOOTMGR] bob_loader\n");

	{ // prepare bob for bobloader
		printf("[BOOTMGR] prepping bob\n");
		SceKernelAllocMemBlockKernelOpt optp;
		optp.size = 0x58;
		optp.attr = 2;
		optp.paddr = 0x1c000000; // tachyon eDRAM
		int tachyon_edram_mb = sceKernelAllocMemBlock("tachyon_edram", 0x10208006, 0x200000, &optp);
		void* tachyon_edram = NULL;
		sceKernelGetMemBlockBase(tachyon_edram_mb, (void**)&tachyon_edram);
		if (!tachyon_edram) {
			printf("[BOOTMGR] E: could not malloc on tachyon edram\n");
			return;
		}

		int bob_size = get_file("os0:bob.bin", NULL, 0, 0);
		if (!bob_size || bob_size < 0) {
			printf("[BOOTMGR] E: could not get bob size\n");
			return;
		}

		memset(tachyon_edram, 0, (uint32_t)bob_size + 0x20);

		if (get_file("os0:bob.bin", tachyon_edram + 0x20, (uint32_t)bob_size, 0) < 0) {
			printf("[BOOTMGR] E: could not read bob\n");
			return;
		}

		bob_info* bobinfo = (bob_info*)tachyon_edram; // some pallocated mem for bobloader args

		// bobloader args
		bobinfo->bob_addr = 0x1C000020; // bob addr
		bobinfo->bob_size = (uint32_t)bob_size; // bob size

		// bob init args
		bobinfo->config.ce_framework_parms_addr[0] = 0x1F850000; // spl framework/only runs at arm interrup
		bobinfo->config.ce_framework_parms_addr[1] = 0; // broombroom framework/runs in a loop when idle
		bobinfo->config.uart_params = (0 << 0x18) | 0x1001A; // uart bus | uart baud
		bobinfo->config.run_tests = 1; // run the test func

		// add brom if exists
		if (get_file("os0:brom.bin", NULL, 0, 0) > 0) {
			printf("[BOOTMGR] found brom, adding brom\n");
			memset(tachyon_edram + 0x00100000, 0, 0x4000);
			if (get_file("os0:brom.bin", tachyon_edram + 0x00100000, 0, 0) < 0)
				printf("[BOOTMGR] W: could not read brom\n");
		}
		
		// add brom if exists
		int alice_size = get_file("os0:alice.bin", NULL, 0, 0);
		if ((alice_size > 0) && (alice_size <= 0x000fc000)) {
			printf("[BOOTMGR] found alice, adding alice\n");
			memset(tachyon_edram + 0x00104000, 0, (uint32_t)alice_size);
			if (get_file("os0:alice.bin", tachyon_edram + 0x00104000, 0, 0) < 0)
				printf("[BOOTMGR] W: could not read alice\n");
		}
	}

	char backup[0x80];
	memcpy(backup, payload_block_addr, 0x80);
	memset(payload_block_addr, 0, 0x300);

	memcpy(payload_block_addr + 0x20, bob_loader_nmp, sizeof(bob_loader_nmp));

	fm_nfo *fmnfo = payload_block_addr;
	fmnfo->magic = 0x14FF;
	fmnfo->status = 0x34;
	fmnfo->codepaddr = 0x1f850020;
	fmnfo->arg = 0x1C000000;

	printf("[BOOTMGR] running bobloader\n");

	smc_custom(0, 0, 0, 0, 0x13c); // run custom code
	
	int ret = (fmnfo->status == 0x69) ? (int)fmnfo->resp : -1;
	
	memcpy(payload_block_addr, backup, 0x80);

	printf("[BOOTMGR] bob_loader ret 0x%X\n", ret);

	while (1) {}; // dont continue boot

	return;
}