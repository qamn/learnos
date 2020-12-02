org 0x7c00 ;汇编时起始地址就是0x7c00

BaseOfStack equ 0x7c00 ;程序从左往右, 栈从右往左

BaseOfLoader	equ	0x1000 ; loader放到0x10000处,则段地址就是0x1000
OffsetOfLoader	equ	0x00

RootDirSectors	equ	14 ; 根目录占的扇区数
SectorNumOfRootDirStart	equ	19 ; 根目录起始扇区号
SectorNumOfFAT1Start	equ	1 ; FAT1起始扇区号
SectorBalance	equ	17	

	jmp	short Label_Start
	nop
	BS_OEMName	db	'MINEboot' ; 厂商名
	BPB_BytesPerSec	dw	512 ; 每扇区字节数
	BPB_SecPerClus	db	1 ;每簇扇区数
	BPB_RsvdSecCnt	dw	1 ; 保留扇区数
	BPB_NumFATs	db	2 ; FAT表份数
	BPB_RootEntCnt	dw	224 ; 根目录最大目录项数
	BPB_TotSec16	dw	2880 ; 总扇区数
	BPB_Media	db	0xf0
	BPB_FATSz16	dw	9 ; 每个FAT扇区数
	BPB_SecPerTrk	dw	18 ; 每磁道扇区数
	BPB_NumHeads	dw	2 ; 磁头数
	BPB_HiddSec	dd	0
	BPB_TotSec32	dd	0
	BS_DrvNum	db	0
	BS_Reserved1	db	0
	BS_BootSig	db	0x29
	BS_VolID	dd	0 ; 卷号
	BS_VolLab	db	'boot loader' ; 卷标
	BS_FileSysType	db	'FAT12' ; 文件系统类型

Label_Start:

    ; cs启动时默认是0, 其他段寄存器初始化成0, 栈指针初始化为0x7c00
    mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	BaseOfStack

    ;清屏, AH=06,
    mov	ax,	0600h
	mov	bx,	0700h
	mov	cx,	0
	mov	dx,	0184fh
	int	10h ; 10h号BIOS中断用来操作显示屏

    ; 设置光标,AH=02,DH=列,DL=行,BH=页码
	mov	ax,	0200h
	mov	bx,	0000h
	mov	dx,	0000h
	int	10h

    ; 显示消息, AH=13,AL=写入模式,CX=字符串长度,DH=行号,DL=列号,ES:BP=字符串地址,BH=页码,BL=字符串属性和颜色
	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0000h
	mov	cx,	NoLoaderMessage-StartMessage
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartMessage
	int	10h

    ; 重置软盘
 	xor	ah,	ah
	xor	dl,	dl
	int	13h ; 13h号BIOS中断用来操作软盘

; 查找 "loader.bin" 文件
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
	mov	si,	LoaderFileName
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
	and	di,	0ffe0h
	add	di,	20h
	mov	si,	LoaderFileName
	jmp	Label_Search_For_LoaderBin

Label_Goto_Next_Sector_In_Root_Dir:
	add	word	[SectorNo],	1
	jmp	Lable_Search_In_Root_Dir_Begin

;未找到则显示信息
Label_No_LoaderBin:
	mov	ax,	1301h
	mov	bx,	008ch
	mov	dx,	0100h
	mov	cx,	21
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderMessage
	int	10h
	jmp	$

;找到则加载到 0x10000处
Label_FileName_Found:
	mov	ax,	RootDirSectors
	and	di,	0ffe0h
	add	di,	01ah
	mov	cx,	word	[es:di]
	push	cx
	add	cx,	ax
	add	cx,	SectorBalance
	mov	ax,	BaseOfLoader
	mov	es,	ax
	mov	bx,	OffsetOfLoader
	mov	ax,	cx

Label_Go_On_Loading_File:
	push	ax
	push	bx
	mov	ah,	0eh
	mov	al,	'.'
	mov	bl,	0fh
	int	10h
	pop	bx
	pop	ax

	mov	cl,	1
	call	Func_ReadOneSector
	pop	ax
	call	Func_GetFATEntry
	cmp	ax,	0fffh
	jz	Label_File_Loaded
	push	ax
	mov	dx,	RootDirSectors
	add	ax,	dx
	add	ax,	SectorBalance
	add	bx,	[BPB_BytesPerSec]
	jmp	Label_Go_On_Loading_File

Label_File_Loaded:
	jmp	BaseOfLoader:OffsetOfLoader

;函数,读一个扇区
Func_ReadOneSector:
	push	bp
	mov	bp,	sp
	sub	esp,	2
	mov	byte	[bp - 2],	cl
	push	bx
	mov	bl,	[BPB_SecPerTrk]
	div	bl
	inc	ah
	mov	cl,	ah
	mov	dh,	al
	shr	al,	1
	mov	ch,	al
	and	dh,	1
	pop	bx
	mov	dl,	[BS_DrvNum]
Label_Go_On_Reading:
	mov	ah,	2
	mov	al,	byte	[bp - 2]
	int	13h
	jc	Label_Go_On_Reading
	add	esp,	2
	pop	bp
	ret

;函数,获取FAT项
Func_GetFATEntry:
	push	es
	push	bx
	push	ax
	mov	ax,	00
	mov	es,	ax
	pop	ax
	mov	byte	[Odd],	0
	mov	bx,	3
	mul	bx
	mov	bx,	2
	div	bx
	cmp	dx,	0
	jz	Label_Even
	mov	byte	[Odd],	1

Label_Even:
	xor	dx,	dx
	mov	bx,	[BPB_BytesPerSec]
	div	bx
	push	dx
	mov	bx,	8000h
	add	ax,	SectorNumOfFAT1Start
	mov	cl,	2
	call	Func_ReadOneSector
	
	pop	dx
	add	bx,	dx
	mov	ax,	[es:bx]
	cmp	byte	[Odd],	1
	jnz	Label_Even_2
	shr	ax,	4

Label_Even_2:
	and	ax,	0fffh
	pop	bx
	pop	es
	ret	

;临时变量
RootDirSizeForLoop:	dw	RootDirSectors
SectorNo:		dw	0
Odd:			db	0

;要显示的字符串
StartMessage: db "Start_Boot_1_2_3_4_5"
NoLoaderMessage:	db	"ERROR:No LOADER Found"
LoaderFileName:		db	"LOADER  BIN",0


times 510-($-$$) db 0 ; 填满512字节
dw 0xaa55 ; 启动扇区的标志
