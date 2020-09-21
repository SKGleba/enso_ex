.arch armv7a

.section .vitalink.fstubs.SceKblForKernel,"ax",%progbits

	.align 4
	.global KblGetDipsw204And205Stat
	.type KblGetDipsw204And205Stat, %function
KblGetDipsw204And205Stat:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xD0FC2991
	.word 0xC011935A
	.align 4

