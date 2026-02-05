	.file	"dynamic_for_asm_test.cpp"
	.intel_syntax noprefix
	.text
	.p2align 4
	.globl	_Z15dynamic_for_summm
	.type	_Z15dynamic_for_summm, @function
_Z15dynamic_for_summm:
.LFB1259:
	.cfi_startproc
	push	r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	push	r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	push	r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	push	r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	push	rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	push	rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	cmp	rsi, rdi
	jnb	.L42
	mov	rcx, rdi
	mov	r12, -7
	mov	rbp, -6
	mov	rbx, -5
	sub	rcx, rsi
	mov	r11, -4
	mov	r10, -3
	mov	r9, -2
	mov	rsi, -8
	mov	r8, -1
	.p2align 4
	.p2align 3
.L4:
	cmp	rcx, 7
	jbe	.L43
	lea	rax, -8[rcx]
	mov	r13, rax
	shr	r13, 3
	cmp	rax, 23
	jbe	.L17
	movd	xmm5, r8d
	movq	xmm2, rsi
	movd	xmm6, r9d
	lea	rax, 0[0+rsi*4]
	lea	rdx, 1[r13]
	punpcklqdq	xmm2, xmm2
	pshufd	xmm11, xmm5, 0
	movq	xmm4, rax
	movd	xmm5, r11d
	lea	rax, [rsi+rsi]
	mov	r14, rdx
	movq	xmm0, rdi
	movq	xmm3, rax
	movd	xmm7, r10d
	xor	eax, eax
	pand	xmm2, XMMWORD PTR .LC0[rip]
	punpcklqdq	xmm0, xmm0
	shr	r14, 2
	pshufd	xmm10, xmm6, 0
	pshufd	xmm8, xmm5, 0
	movd	xmm6, ebx
	movd	xmm5, ebp
	punpcklqdq	xmm4, xmm4
	paddq	xmm2, xmm0
	pshufd	xmm9, xmm7, 0
	punpcklqdq	xmm3, xmm3
	pshufd	xmm7, xmm6, 0
	pxor	xmm0, xmm0
	pshufd	xmm6, xmm5, 0
	movd	xmm5, r12d
	pshufd	xmm5, xmm5, 0
	.p2align 4
	.p2align 3
.L8:
	movdqa	xmm12, xmm2
	movdqa	xmm1, xmm2
	add	rax, 1
	paddq	xmm12, xmm3
	paddq	xmm2, xmm4
	shufps	xmm1, xmm12, 136
	movdqa	xmm12, xmm11
	paddd	xmm0, xmm1
	paddd	xmm12, xmm1
	paddd	xmm0, xmm12
	movdqa	xmm12, xmm10
	paddd	xmm12, xmm1
	paddd	xmm0, xmm12
	movdqa	xmm12, xmm9
	paddd	xmm12, xmm1
	paddd	xmm0, xmm12
	movdqa	xmm12, xmm8
	paddd	xmm12, xmm1
	paddd	xmm0, xmm12
	movdqa	xmm12, xmm7
	paddd	xmm12, xmm1
	paddd	xmm0, xmm12
	movdqa	xmm12, xmm6
	paddd	xmm12, xmm1
	paddd	xmm1, xmm5
	paddd	xmm0, xmm12
	paddd	xmm0, xmm1
	cmp	r14, rax
	jne	.L8
	movdqa	xmm1, xmm0
	psrldq	xmm1, 8
	paddd	xmm0, xmm1
	movdqa	xmm1, xmm0
	psrldq	xmm1, 4
	paddd	xmm0, xmm1
	movd	eax, xmm0
	test	dl, 3
	je	.L9
	and	rdx, -4
	mov	r14, rdx
	imul	rdx, rsi
	neg	r14
	lea	r14, [rcx+r14*8]
	add	rdx, rdi
.L7:
	lea	eax, [rax+rdx*2]
	lea	r15, [r9+rdx]
	add	eax, r8d
	add	eax, r15d
	lea	r15, [r10+rdx]
	add	eax, r15d
	lea	r15, [r11+rdx]
	add	eax, r15d
	lea	r15, [rbx+rdx]
	add	eax, r15d
	lea	r15, 0[rbp+rdx]
	add	eax, r15d
	lea	r15, [r12+rdx]
	add	eax, r15d
	lea	r15, -8[r14]
	cmp	r15, 7
	jbe	.L9
	add	rdx, rsi
	sub	r14, 16
	lea	eax, [rax+rdx*2]
	lea	r15, [rdx+r9]
	add	eax, r8d
	add	eax, r15d
	lea	r15, [rdx+r10]
	add	eax, r15d
	lea	r15, [rdx+r11]
	add	eax, r15d
	lea	r15, [rdx+rbx]
	add	eax, r15d
	lea	r15, [rdx+rbp]
	add	eax, r15d
	lea	r15, [rdx+r12]
	add	eax, r15d
	cmp	r14, 7
	jbe	.L9
	add	rdx, rsi
	lea	eax, [rax+rdx*2]
	lea	r14, [rdx+r9]
	add	eax, r8d
	add	eax, r14d
	lea	r14, [rdx+r10]
	add	eax, r14d
	lea	r14, [rdx+r11]
	add	eax, r14d
	lea	r14, [rdx+rbx]
	add	eax, r14d
	lea	r14, [rdx+rbp]
	add	rdx, r12
	add	eax, r14d
	add	eax, edx
.L9:
	and	ecx, 7
	jne	.L44
.L1:
	pop	rbx
	.cfi_remember_state
	.cfi_def_cfa_offset 48
	pop	rbp
	.cfi_def_cfa_offset 40
	pop	r12
	.cfi_def_cfa_offset 32
	pop	r13
	.cfi_def_cfa_offset 24
	pop	r14
	.cfi_def_cfa_offset 16
	pop	r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L42:
	.cfi_restore_state
	cmp	rdi, rsi
	jnb	.L16
	mov	rcx, rsi
	mov	r12d, 7
	mov	ebp, 6
	mov	ebx, 5
	sub	rcx, rdi
	mov	r11d, 4
	mov	esi, 8
	mov	r10d, 3
	mov	r9d, 2
	mov	r8d, 1
	jmp	.L4
	.p2align 4,,10
	.p2align 3
.L17:
	mov	r14, rcx
	mov	rdx, rdi
	xor	eax, eax
	jmp	.L7
	.p2align 4,,10
	.p2align 3
.L44:
	imul	r13, rsi
	add	rdi, rsi
	lea	rdx, 0[r13+rdi]
	add	eax, edx
	cmp	rcx, 7
	je	.L45
	cmp	rcx, 6
	je	.L46
	cmp	rcx, 4
	je	.L12
	cmp	rcx, 5
	je	.L13
	cmp	rcx, 2
	je	.L14
	lea	rsi, [r8+rdx]
	add	rdx, r9
	add	esi, eax
	add	edx, esi
	cmp	rcx, 3
	cmove	eax, edx
	jmp	.L1
.L43:
	mov	eax, edi
	cmp	rcx, 1
	je	.L1
	lea	rdx, [rdi+r8]
	lea	eax, [rdi+rdx]
	cmp	rcx, 2
	je	.L1
	add	rdx, r8
	add	eax, edx
	cmp	rcx, 3
	je	.L1
	add	rdx, r8
	add	eax, edx
	cmp	rcx, 4
	je	.L1
	add	rdx, r8
	add	eax, edx
	cmp	rcx, 5
	je	.L1
	add	rdx, r8
	add	eax, edx
	add	rdx, r8
	add	edx, eax
	cmp	rcx, 7
	cmove	eax, edx
	jmp	.L1
.L12:
	add	r8, rdx
	add	r9, rdx
	add	r10, rdx
	add	r8d, eax
	lea	eax, [r8+r9]
	add	eax, r10d
	jmp	.L1
.L45:
	add	r8, rdx
	add	r9, rdx
	add	r10, rdx
	add	r11, rdx
	add	eax, r8d
	add	rbx, rdx
	add	rbp, rdx
	add	eax, r9d
	add	eax, r10d
	add	eax, r11d
	add	eax, ebx
	add	eax, ebp
	jmp	.L1
.L46:
	add	r8, rdx
	add	r9, rdx
	add	r10, rdx
	add	r11, rdx
	add	eax, r8d
	add	rbx, rdx
	add	eax, r9d
	add	eax, r10d
	add	eax, r11d
	add	eax, ebx
	jmp	.L1
.L14:
	add	r8, rdx
	add	eax, r8d
	jmp	.L1
.L13:
	add	r8, rdx
	add	r9, rdx
	add	r10, rdx
	add	r11, rdx
	add	eax, r8d
	add	eax, r9d
	add	eax, r10d
	add	eax, r11d
	jmp	.L1
.L16:
	xor	eax, eax
	jmp	.L1
	.cfi_endproc
.LFE1259:
	.size	_Z15dynamic_for_summm, .-_Z15dynamic_for_summm
	.p2align 4
	.globl	_Z10manual_summm
	.type	_Z10manual_summm, @function
_Z10manual_summm:
.LFB1261:
	.cfi_startproc
	cmp	rdi, rsi
	jnb	.L53
	mov	rcx, rsi
	sub	rcx, rdi
	lea	rax, -1[rcx]
	cmp	rax, 2
	jbe	.L54
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
	paddq	xmm0, XMMWORD PTR .LC1[rip]
	punpcklqdq	xmm5, xmm5
	punpcklqdq	xmm4, xmm4
	.p2align 6
	.p2align 4
	.p2align 3
.L50:
	movdqa	xmm3, xmm0
	movdqa	xmm2, xmm0
	paddq	xmm0, xmm4
	add	rax, 1
	paddq	xmm3, xmm5
	shufps	xmm2, xmm3, 136
	paddd	xmm1, xmm2
	cmp	rax, rdx
	jne	.L50
	movdqa	xmm0, xmm1
	psrldq	xmm0, 8
	paddd	xmm1, xmm0
	movdqa	xmm0, xmm1
	psrldq	xmm0, 4
	paddd	xmm1, xmm0
	movd	eax, xmm1
	test	cl, 3
	je	.L47
	and	rcx, -4
	add	rdi, rcx
.L49:
	lea	rdx, 1[rdi]
	add	eax, edi
	cmp	rdx, rsi
	jnb	.L47
	add	eax, edx
	add	rdi, 2
	lea	edx, [rax+rdi]
	cmp	rdi, rsi
	cmovb	eax, edx
	ret
	.p2align 4,,10
	.p2align 3
.L53:
	xor	eax, eax
.L47:
	ret
.L54:
	xor	eax, eax
	jmp	.L49
	.cfi_endproc
.LFE1261:
	.size	_Z10manual_summm, .-_Z10manual_summm
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB1262:
	.cfi_startproc
	mov	eax, 90
	ret
	.cfi_endproc
.LFE1262:
	.size	main, .-main
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC0:
	.quad	0
	.quad	-1
	.align 16
.LC1:
	.quad	0
	.quad	1
	.ident	"GCC: (Debian 15.2.0-12) 15.2.0"
	.section	.note.GNU-stack,"",@progbits
