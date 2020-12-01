org 10000h

    ; 跳转到loader
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ax,	0x00
	mov	ss,	ax
	mov	sp,	0x7c00

    ; 显示加载了loader
	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0200h		;第二行
	mov	cx,	18
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartLoaderMessage
	int	10h

	jmp	$

StartLoaderMessage:	db	"Start Loader......"