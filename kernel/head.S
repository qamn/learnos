/*==================================================================*/
#include "lib/linkage.h"
.section .text

.globl _start
_start: /* 执行到这里, 已经运行在临时64位模式, 页表在90000处, 统一映射 */
    /* 设置段寄存器 */
	mov	$0x10,	%ax /* 10是数据段索引 */
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%fs
	mov	%ax,	%ss
	mov	$0x7E00,	%esp

    /* 加载新的 GDTR */
    lgdt	GDT_POINTER(%rip)/* 相对当前指令来寻址 */

    /* 加载新的 IDTR */
    lidt	IDT_POINTER(%rip)

    /* 切换GTD之后, 重新设置各个段寄存器 */
	mov	$0x10,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%fs
	mov	%ax,	%gs
	mov	%ax,	%ss
	movq	$0x7E00,	%rsp

    /* 重新加载 CR3, 更新页表 */
	movq	$0x101000,	%rax
	movq	%rax,		%cr3

/* 用lretq跳转, 从临时64位模式跳转到正式的64位模式 */
	movq	switch_seg(%rip),	%rax
	pushq	$0x08
	pushq	%rax
	lretq
switch_seg:
	.quad	entry64
entry64:
    /* 重新设置段寄存器 */
	movq	$0x10,	%rax
	movq	%rax,	%ds
	movq	%rax,	%es
	movq	%rax,	%gs
	movq	%rax,	%ss
	movq	$0xffff800000007E00,	%rsp		/* rsp address */

/* 设置GDT中的TSS描述符 */
setup_TSS64:
	leaq	TSS64_Table(%rip),	%rdx
	xorq	%rax,	%rax
	xorq	%rcx,	%rcx
	movq	$0x89,	%rax
	shlq	$40,	%rax
	movl	%edx,	%ecx
	shrl	$24,	%ecx
	shlq	$56,	%rcx
	addq	%rcx,	%rax
	xorq	%rcx,	%rcx
	movl	%edx,	%ecx
	andl	$0xffffff,	%ecx
	shlq	$16,	%rcx
	addq	%rcx,	%rax
	addq	$103,	%rax
	leaq	GDT_Table(%rip),	%rdi
	movq	%rax,	64(%rdi)
	shrq	$32,	%rdx
	movq	%rdx,	72(%rdi)

	// mov	$0x40,	%ax /* 4*16/8=8, 就是GDT中索引为8的描述符, 就是TSS的描述符, 注释掉, 是因为会在main中设置*/
	// ltr	%ax /* 将TSS描述符加载到TR寄存器 */

    /* 跳转到C语言的内核启动函数 */
	movq	go_to_kernel(%rip),	%rax		/* movq address */
	pushq	$0x08
	pushq	%rax
	lretq
go_to_kernel:
	.quad	Start_Kernel

ENTRY(_stack_start) /* init任务的末尾作为栈的基址，向左生长。在main.c中extern使用 */
	.quad	init_task_union + 32768

/* 页表 */
.align 8

.org	0x1000
__PML4E:
	.quad	0x102007
	.fill	255,8,0
	.quad	0x102007
	.fill	255,8,0

.org	0x2000
__PDPTE:
	.quad	0x103003
	.fill	511,8,0

.org	0x3000
__PDE:
	.quad	0x000083	
	.quad	0x200083
	.quad	0x400083
	.quad	0x600083
	.quad	0x800083
	.quad	0xe0000083		/*0x a00000*/
	.quad	0xe0200083
	.quad	0xe0400083
	.quad	0xe0600083		/*0x1000000*/
	.quad	0xe0800083
	.quad	0xe0a00083
	.quad	0xe0c00083
	.quad	0xe0e00083
	.fill	499,8,0


/*==================================================================*/
.section .data

/* GDT */
.globl GDT_Table
GDT_Table:
	.quad	0x0000000000000000			/*0	NULL descriptor		       	00*/
	.quad	0x0020980000000000			/*1	KERNEL	Code	64-bit	Segment	08*/
	.quad	0x0000920000000000			/*2	KERNEL	Data	64-bit	Segment	10*/
	.quad	0x0020f80000000000			/*3	USER	Code	64-bit	Segment	18*/
	.quad	0x0000f20000000000			/*4	USER	Data	64-bit	Segment	20*/
	.quad	0x00cf9a000000ffff			/*5	KERNEL	Code	32-bit	Segment	28*/
	.quad	0x00cf92000000ffff			/*6	KERNEL	Data	32-bit	Segment	30*/
	.fill	10,8,0					/*8 ~ 9	TSS (jmp one segment <7>) in long-mode 128-bit 40*/
GDT_END:
GDT_POINTER:
GDT_LIMIT:	.word	GDT_END - GDT_Table - 1
GDT_BASE:	.quad	GDT_Table

/* IDT */
.globl IDT_Table
IDT_Table:
	.fill  512,8,0
IDT_END:
IDT_POINTER:
IDT_LIMIT:	.word	IDT_END - IDT_Table - 1
IDT_BASE:	.quad	IDT_Table

/* TSS,任务状态段 */
.globl	TSS64_Table
TSS64_Table:
	.fill  13,8,0
TSS64_END:
TSS64_POINTER:
TSS64_LIMIT:	.word	TSS64_END - TSS64_Table - 1
TSS64_BASE:	.quad	TSS64_Table