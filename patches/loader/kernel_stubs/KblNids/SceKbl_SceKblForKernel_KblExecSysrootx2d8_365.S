.arch armv7a

.section .vitalink.fstubs.SceKblForKernel,"ax",%progbits

	.align 4
	.global KblExecSysrootx2d8
	.type KblExecSysrootx2d8, %function
KblExecSysrootx2d8:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xD0FC2991
	.word 0xC7B77991
	.align 4

