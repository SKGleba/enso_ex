.arch armv7a

.section .vitalink.fstubs.SceKblForKernel,"ax",%progbits

	.align 4
	.global KblGetHwConfig
	.type KblGetHwConfig, %function
KblGetHwConfig:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xD0FC2991
	.word 0xD3A516D5
	.align 4

