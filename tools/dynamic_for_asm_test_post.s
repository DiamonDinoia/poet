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
	cmp	rax, 2
	jbe	.L8
	movq	xmm0, rdi
	mov	r8d, 2
	mov	rdx, rcx
	xor	eax, eax
	mov	r9d, 4
	punpcklqdq	xmm0, xmm0
	shr	rdx, 2
	pxor	xmm1, xmm1
	movq	xmm5, r8
	movq	xmm4, r9
	paddq	xmm0, XMMWORD PTR .LC0[rip]
	punpcklqdq	xmm5, xmm5
	punpcklqdq	xmm4, xmm4
	.p2align 6
	.p2align 4
	.p2align 3
.L4:
	movdqa	xmm3, xmm0
	movdqa	xmm2, xmm0
	paddq	xmm0, xmm4
	add	rax, 1
	paddq	xmm3, xmm5
	shufps	xmm2, xmm3, 136
	paddd	xmm1, xmm2
	cmp	rax, rdx
	jne	.L4
	movdqa	xmm0, xmm1
	psrldq	xmm0, 8
	paddd	xmm1, xmm0
	movdqa	xmm0, xmm1
	psrldq	xmm0, 4
	paddd	xmm1, xmm0
	movd	eax, xmm1
	test	cl, 3
	je	.L1
	and	rcx, -4
	add	rdi, rcx
.L3:
	lea	rdx, 1[rdi]
	add	eax, edi
	cmp	rdx, rsi
	jnb	.L1
	add	eax, edx
	add	rdi, 2
	lea	edx, [rax+rdi]
	cmp	rdi, rsi
	cmovb	eax, edx
	ret
	.p2align 4,,10
	.p2align 3
.L7:
	xor	eax, eax
.L1:
	ret
.L8:
	xor	eax, eax
	jmp	.L3
	.cfi_endproc
.LFE1262:
	.size	_Z10manual_summm, .-_Z10manual_summm
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC4:
	.string	"dynamic_for_sum<8>: "
.LC5:
	.string	" s, result: "
.LC6:
	.string	"\n"
.LC7:
	.string	"dynamic_for_sum<4>: "
.LC8:
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
	mov	edx, 20
	lea	rsi, .LC4[rip]
	lea	rdi, _ZSt4cout[rip]
	sub	rax, rbx
	pxor	xmm0, xmm0
	cvtsi2sd	xmm0, rax
	divsd	xmm0, QWORD PTR .LC3[rip]
	movsd	QWORD PTR 8[rsp], xmm0
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movsd	xmm0, QWORD PTR 8[rsp]
	lea	rdi, _ZSt4cout[rip]
	call	_ZNSo9_M_insertIdEERSoT_@PLT
	mov	edx, 12
	lea	rsi, .LC5[rip]
	mov	rbx, rax
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	mov	rdi, rbx
	movabs	rsi, 167731200000
	call	_ZNSo9_M_insertIxEERSoT_@PLT
	mov	edx, 1
	lea	rsi, .LC6[rip]
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	call	_ZNSt6chrono3_V212system_clock3nowEv@PLT
	mov	rbx, rax
	call	_ZNSt6chrono3_V212system_clock3nowEv@PLT
	mov	edx, 20
	lea	rsi, .LC7[rip]
	lea	rdi, _ZSt4cout[rip]
	sub	rax, rbx
	pxor	xmm0, xmm0
	cvtsi2sd	xmm0, rax
	divsd	xmm0, QWORD PTR .LC3[rip]
	movsd	QWORD PTR 8[rsp], xmm0
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movsd	xmm0, QWORD PTR 8[rsp]
	lea	rdi, _ZSt4cout[rip]
	call	_ZNSo9_M_insertIdEERSoT_@PLT
	mov	edx, 12
	lea	rsi, .LC5[rip]
	mov	rbx, rax
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	mov	rdi, rbx
	movabs	rsi, 167731200000
	call	_ZNSo9_M_insertIxEERSoT_@PLT
	mov	edx, 1
	lea	rsi, .LC6[rip]
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	call	_ZNSt6chrono3_V212system_clock3nowEv@PLT
	mov	rbx, rax
	call	_ZNSt6chrono3_V212system_clock3nowEv@PLT
	mov	edx, 16
	lea	rsi, .LC8[rip]
	lea	rdi, _ZSt4cout[rip]
	sub	rax, rbx
	pxor	xmm0, xmm0
	cvtsi2sd	xmm0, rax
	divsd	xmm0, QWORD PTR .LC3[rip]
	movsd	QWORD PTR 8[rsp], xmm0
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movsd	xmm0, QWORD PTR 8[rsp]
	lea	rdi, _ZSt4cout[rip]
	call	_ZNSo9_M_insertIdEERSoT_@PLT
	mov	edx, 12
	lea	rsi, .LC5[rip]
	mov	rbx, rax
	mov	rdi, rax
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	mov	rdi, rbx
	movabs	rsi, 167731200000
	call	_ZNSo9_M_insertIxEERSoT_@PLT
	mov	edx, 1
	lea	rsi, .LC6[rip]
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
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC0:
	.quad	0
	.quad	1
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC3:
	.long	0
	.long	1104006501
	.ident	"GCC: (Debian 15.2.0-12) 15.2.0"
	.section	.note.GNU-stack,"",@progbits
