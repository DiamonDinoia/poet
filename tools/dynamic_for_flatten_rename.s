	.file	"dynamic_for_asm_test.cpp"
	.intel_syntax noprefix
	.text
#APP
	.globl _ZSt21ios_base_library_initv
#NO_APP
	.p2align 4
	.globl	_Z10manual_summm
	.type	_Z10manual_summm, @function
_Z10manual_summm:
.LFB1262:
	.cfi_startproc
	cmp	rdi, rsi
	jnb	.L7
	mov	rcx, rsi
	sub	rcx, rdi
	lea	rax, -1[rcx]
	cmp	rax, 6
	jbe	.L8
	vmovq	xmm6, rdi
	mov	rdx, rcx
	vpxor	xmm3, xmm3, xmm3
	xor	r8d, r8d
	vpbroadcastq	ymm2, xmm6
	vpaddq	ymm11, ymm2, YMMWORD PTR .LC0[rip]
	shr	rdx, 3
	vpbroadcastq	ymm5, QWORD PTR .LC3[rip]
	vpbroadcastq	ymm4, QWORD PTR .LC4[rip]
	.p2align 6
	.p2align 4,,10
	.p2align 3
.L4:
	vpaddq	ymm0, ymm11, ymm5
	add	r8, 1
	vperm2i128	ymm1, ymm11, ymm0, 32
	vperm2i128	ymm7, ymm11, ymm0, 49
	vpaddq	ymm11, ymm11, ymm4
	cmp	r8, rdx
	vpshufd	ymm8, ymm1, 216
	vpshufd	ymm9, ymm7, 216
	vpunpcklqdq	ymm10, ymm8, ymm9
	vpaddd	ymm3, ymm3, ymm10
	jne	.L4
	vextracti128	xmm12, ymm3, 0x1
	test	cl, 7
	vpaddd	xmm13, xmm12, xmm3
	vpsrldq	xmm14, xmm13, 8
	vpaddd	xmm15, xmm13, xmm14
	vpsrldq	xmm6, xmm15, 4
	vpaddd	xmm2, xmm15, xmm6
	vmovd	eax, xmm2
	je	.L13
	and	rcx, -8
	add	rdi, rcx
	vzeroupper
.L3:
	lea	r9, 1[rdi]
	add	eax, edi
	cmp	r9, rsi
	jnb	.L1
	lea	r10, 2[rdi]
	add	eax, r9d
	cmp	r10, rsi
	jnb	.L1
	lea	r11, 3[rdi]
	add	eax, r10d
	cmp	r11, rsi
	jnb	.L1
	lea	rcx, 4[rdi]
	add	eax, r11d
	cmp	rcx, rsi
	jnb	.L1
	lea	rdx, 5[rdi]
	add	eax, ecx
	cmp	rdx, rsi
	jnb	.L1
	add	eax, edx
	add	rdi, 6
	lea	r8d, [rax+rdi]
	cmp	rdi, rsi
	cmovb	eax, r8d
	ret
	.p2align 4,,10
	.p2align 3
.L13:
	vzeroupper
.L1:
	ret
	.p2align 4,,10
	.p2align 3
.L7:
	xor	eax, eax
	ret
.L8:
	xor	eax, eax
	jmp	.L3
	.cfi_endproc
.LFE1262:
	.size	_Z10manual_summm, .-_Z10manual_summm
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC6:
	.string	"dynamic_for_sum<8>: "
.LC7:
	.string	" s, result: "
.LC8:
	.string	"\n"
.LC9:
	.string	"dynamic_for_sum<4>: "
.LC10:
	.string	"manual_sum:     "
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB2790:
	.cfi_startproc
	push	rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	sub	rsp, 32
	.cfi_def_cfa_offset 48
	mov	DWORD PTR 28[rsp], 16773120
	mov	eax, DWORD PTR 28[rsp]
	call	_ZNSt6chrono3_V212system_clock3nowEv@PLT
	mov	rbx, rax
	call	_ZNSt6chrono3_V212system_clock3nowEv@PLT
	vxorpd	xmm1, xmm1, xmm1
	mov	edx, 20
	lea	rsi, .LC6[rip]
	sub	rax, rbx
	lea	rdi, _ZSt4cout[rip]
	vcvtsi2sd	xmm0, xmm1, rax
	vdivsd	xmm2, xmm0, QWORD PTR .LC5[rip]
	vmovsd	QWORD PTR 8[rsp], xmm2
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	vmovsd	xmm0, QWORD PTR 8[rsp]
	lea	rdi, _ZSt4cout[rip]
	call	_ZNSo9_M_insertIdEERSoT_@PLT
	mov	edx, 12
	lea	rsi, .LC7[rip]
	mov	rbx, rax
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	mov	rdi, rbx
	movabs	rsi, 167731200000
	call	_ZNSo9_M_insertIxEERSoT_@PLT
	mov	edx, 1
	lea	rsi, .LC8[rip]
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	call	_ZNSt6chrono3_V212system_clock3nowEv@PLT
	mov	rbx, rax
	call	_ZNSt6chrono3_V212system_clock3nowEv@PLT
	vxorpd	xmm3, xmm3, xmm3
	mov	edx, 20
	lea	rsi, .LC9[rip]
	sub	rax, rbx
	lea	rdi, _ZSt4cout[rip]
	vcvtsi2sd	xmm4, xmm3, rax
	vdivsd	xmm5, xmm4, QWORD PTR .LC5[rip]
	vmovsd	QWORD PTR 8[rsp], xmm5
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	vmovsd	xmm0, QWORD PTR 8[rsp]
	lea	rdi, _ZSt4cout[rip]
	call	_ZNSo9_M_insertIdEERSoT_@PLT
	mov	edx, 12
	lea	rsi, .LC7[rip]
	mov	rbx, rax
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	mov	rdi, rbx
	movabs	rsi, 167731200000
	call	_ZNSo9_M_insertIxEERSoT_@PLT
	mov	edx, 1
	lea	rsi, .LC8[rip]
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	call	_ZNSt6chrono3_V212system_clock3nowEv@PLT
	mov	rbx, rax
	call	_ZNSt6chrono3_V212system_clock3nowEv@PLT
	vxorpd	xmm6, xmm6, xmm6
	mov	edx, 16
	lea	rsi, .LC10[rip]
	sub	rax, rbx
	lea	rdi, _ZSt4cout[rip]
	vcvtsi2sd	xmm7, xmm6, rax
	vdivsd	xmm8, xmm7, QWORD PTR .LC5[rip]
	vmovsd	QWORD PTR 8[rsp], xmm8
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	vmovsd	xmm0, QWORD PTR 8[rsp]
	lea	rdi, _ZSt4cout[rip]
	call	_ZNSo9_M_insertIdEERSoT_@PLT
	mov	edx, 12
	lea	rsi, .LC7[rip]
	mov	rbx, rax
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	mov	rdi, rbx
	movabs	rsi, 167731200000
	call	_ZNSo9_M_insertIxEERSoT_@PLT
	mov	edx, 1
	lea	rsi, .LC8[rip]
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	add	rsp, 32
	.cfi_def_cfa_offset 16
	xor	eax, eax
	pop	rbx
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE2790:
	.size	main, .-main
	.section	.rodata.cst32,"aM",@progbits,32
	.align 32
.LC0:
	.quad	0
	.quad	1
	.quad	2
	.quad	3
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC3:
	.quad	4
	.align 8
.LC4:
	.quad	8
	.align 8
.LC5:
	.long	0
	.long	1104006501
	.ident	"GCC: (Debian 15.2.0-12) 15.2.0"
	.section	.note.GNU-stack,"",@progbits
