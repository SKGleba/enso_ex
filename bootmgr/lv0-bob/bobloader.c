static const unsigned char bob_loader_nmp[] = {
  0xc0, 0x6f, 0x1a, 0x7b, 0x00, 0x53, 0x06, 0x4b, 0x1e, 0xc0, 0x04, 0x00,
  0x03, 0x03, 0x27, 0xa0, 0x21, 0xc2, 0x10, 0x1c, 0x01, 0xc3, 0xff, 0x0f,
  0x09, 0xe3, 0x08, 0x00, 0x2e, 0x09, 0x21, 0xca, 0xf5, 0xe3, 0xa4, 0xca,
  0x00, 0xc0, 0xa0, 0x92, 0x0a, 0x09, 0x10, 0x62, 0x20, 0x61, 0xb0, 0xd3,
  0x00, 0x04, 0x3f, 0x10, 0x1e, 0x02, 0x21, 0xc9, 0x04, 0x00, 0x32, 0x92,
  0x2e, 0x00, 0x92, 0x93, 0x10, 0x63, 0x2a, 0x00, 0xc4, 0xbf, 0x00, 0x00,
  0xf0, 0xff, 0x1f, 0x00
};

typedef struct bob_info {
	uint32_t bob_addr;
	uint32_t bob_size;
	uint32_t bob_config[];
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

		memset(tachyon_edram, 0, (uint32_t)bob_size);

		if (get_file("os0:bob.bin", tachyon_edram + 0x10, (uint32_t)bob_size, 0) < 0) {
			printf("[BOOTMGR] E: could not read bob\n");
			return;
		}

		bob_info* bobinfo = (bob_info*)tachyon_edram; // some pallocated mem for bobloader args

		// bobloader args
		bobinfo->bob_addr = 0x1C000010; // bob addr
		bobinfo->bob_size = (uint32_t)bob_size; // bob size

		// bob init args
		bobinfo->bob_config[0] = 0x1f850000; // spl framework/only runs at arm interrupt
		bobinfo->bob_config[1] = 0; // broombroom framework/runs in a loop when idle

		// add brom if exists
		if (get_file("os0:brom.bin", NULL, 0, 0) > 0) {
			printf("[BOOTMGR] found brom, adding brom\n");
			memset(tachyon_edram + 0x00100000, 0, 0x4000);
			if (get_file("os0:brom.bin", tachyon_edram + 0x00100000, 0, 0) < 0)
				printf("[BOOTMGR] W: could not read brom\n");
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