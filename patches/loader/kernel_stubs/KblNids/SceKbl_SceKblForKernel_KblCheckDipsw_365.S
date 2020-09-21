.arch armv7a

.section .vitalink.fstubs.SceKblForKernel,"ax",%progbits

	.align 4
	.global KblCheckDipsw
	.type KblCheckDipsw, %function
KblCheckDipsw:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xD0FC2991
	.word 0xC8F4DE71
	.align 4

