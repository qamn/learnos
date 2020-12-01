org 10000h

	jmp Label_Start

RootDirSectors	equ	14
SectorNumOfRootDirStart	equ	19
SectorNumOfFAT1Start	equ	1
SectorBalance	equ	17	

	BS_OEMName	db	'MINEboot'
	BPB_BytesPerSec	dw	512
	BPB_SecPerClus	db	1
	BPB_RsvdSecCnt	dw	1
	BPB_NumFATs	db	2
	BPB_RootEntCnt	dw	224
	BPB_TotSec16	dw	2880
	BPB_Media	db	0xf0
	BPB_FATSz16	dw	9
	BPB_SecPerTrk	dw	18
	BPB_NumHeads	dw	2
	BPB_hiddSec	dd	0
	BPB_TotSec32	dd	0
	BS_DrvNum	db	0
	BS_Reserved1	db	0
	BS_BootSig	db	29h
	BS_VolID	dd	0
	BS_VolLab	db	'boot loader'
	BS_FileSysType	db	'FAT12'

BaseOfKernelFile	equ	0x0000 ; kernel的段地址为0
OffsetOfKernelFile	equ	0x100000 ; kernel的偏移是100000
BaseTmpOfKernelAddr	equ	0x0000 ; kernel临时空间的段地址为0
OffsetTmpOfKernelFile	equ	0x7E00 ; kernel临时空间的偏移地址为 7E00
MemoryStructBufferAddr	equ	0x7E00 ; 内存信息结构体缓存的地址


[SECTION gdt]
LABEL_GDT:		
dd	0,0
LABEL_DESC_CODE32:	dd	0x0000FFFF,0x00CF9A00 ; 段地址0,无限长,代码段,只读
LABEL_DESC_DATA32:	dd	0x0000FFFF,0x00CF9200 ; 段地址0,无限长,数据段,可读写
GdtLen	equ	$ - LABEL_GDT ; GDT长度
GdtPtr	dw	GdtLen - 1 ; 长度
		dd	LABEL_GDT ; 地址
SelectorCode32	equ	LABEL_DESC_CODE32 - LABEL_GDT ; 代码段在GDT中的偏移,就是索引
SelectorData32	equ	LABEL_DESC_DATA32 - LABEL_GDT

[SECTION gdt64]
LABEL_GDT64:		
dq	0x0000000000000000
LABEL_DESC_CODE64:	dq	0x0020980000000000
LABEL_DESC_DATA64:	dq	0x0000920000000000
GdtLen64	equ	$ - LABEL_GDT64
GdtPtr64	dw	GdtLen64 - 1
			dd	LABEL_GDT64
SelectorCode64	equ	LABEL_DESC_CODE64 - LABEL_GDT64
SelectorData64	equ	LABEL_DESC_DATA64 - LABEL_GDT64

[SECTION .s16]
[BITS 16]
Label_Start:
	; 设置段寄存器
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ax,	0x00
	mov	ss,	ax
	mov	sp,	0x7c00

	; 显示loader加载信息
	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0200h		;row 2
	mov	cx,	12
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartLoaderMessage
	int	10h

	; 开启20位地址
	push	ax
	in	al,	92h
	or	al,	00000010b
	out	92h,	al
	pop	ax

	cli
	db	0x66
	lgdt	[GdtPtr]	
	mov	eax,	cr0
	or	eax,	1
	mov	cr0,	eax
	mov	ax,	SelectorData32
	mov	fs,	ax
	mov	eax,	cr0
	and	al,	11111110b
	mov	cr0,	eax
	sti

	; 重置软盘
	xor	ah,	ah
	xor	dl,	dl
	int	13h

; 搜索kernel
	mov	word	[SectorNo],	SectorNumOfRootDirStart
Lable_Search_In_Root_Dir_Begin:
	cmp	word	[RootDirSizeForLoop],	0
	jz	Label_No_LoaderBin
	dec	word	[RootDirSizeForLoop]	
	mov	ax,	00h
	mov	es,	ax
	mov	bx,	8000h
	mov	ax,	[SectorNo]
	mov	cl,	1
	call	Func_ReadOneSector
	mov	si,	KernelFileName
	mov	di,	8000h
	cld
	mov	dx,	10h
	
Label_Search_For_LoaderBin:
	cmp	dx,	0
	jz	Label_Goto_Next_Sector_In_Root_Dir
	dec	dx
	mov	cx,	11

Label_Cmp_FileName:
	cmp	cx,	0
	jz	Label_FileName_Found
	dec	cx
	lodsb	
	cmp	al,	byte	[es:di]
	jz	Label_Go_On
	jmp	Label_Different

Label_Go_On:
	inc	di
	jmp	Label_Cmp_FileName

Label_Different:
	and	di,	0FFE0h
	add	di,	20h
	mov	si,	KernelFileName
	jmp	Label_Search_For_LoaderBin

Label_Goto_Next_Sector_In_Root_Dir:
	add	word	[SectorNo],	1
	jmp	Lable_Search_In_Root_Dir_Begin

; 未找到显示信息
Label_No_LoaderBin:
	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0300h		;row 3
	mov	cx,	21
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderMessage
	int	10h
	jmp	$

; 找到就加载
Label_FileName_Found:
	mov	ax,	RootDirSectors
	and	di,	0FFE0h
	add	di,	01Ah
	mov	cx,	word	[es:di]
	push	cx
	add	cx,	ax
	add	cx,	SectorBalance
	mov	eax,	BaseTmpOfKernelAddr	;BaseOfKernelFile
	mov	es,	eax
	mov	bx,	OffsetTmpOfKernelFile	;OffsetOfKernelFile
	mov	ax,	cx

Label_Go_On_Loading_File:
	push	ax
	push	bx
	mov	ah,	0Eh
	mov	al,	'.'
	mov	bl,	0Fh
	int	10h
	pop	bx
	pop	ax

	mov	cl,	1
	call	Func_ReadOneSector
	pop	ax
