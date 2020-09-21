.arch armv7a

.section .vitalink.fstubs.SceKblForKernel,"ax",%progbits

	.align 4
	.global KblParamx2dAnd1
	.type KblParamx2dAnd1, %function
KblParamx2dAnd1:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xD0FC2991
	.word 0xCE94F329
	.align 4

