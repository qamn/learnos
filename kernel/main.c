#include "print/printk.h"
#include "interrupt/gate.h"
#include "interrupt/trap.h"

void Start_Kernel(void)
{
	int *addr = (int *)0xffff800000a00000; // 显示器的帧缓冲地址
	int i;

	Pos.XResolution = 1440;
	Pos.YResolution = 900;

	Pos.XPosition = 0;
	Pos.YPosition = 0;

	Pos.XCharSize = 8;
	Pos.YCharSize = 16;

	Pos.FB_addr = (int *)0xffff800000a00000;
	Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4);

	color_printk(YELLOW, BLACK, "Hello World!\n");
	color_printk(YELLOW, BLACK, "This is a number:%d!\n",10);

	load_TR(8);

	color_printk(YELLOW, BLACK, "tr loaded\n");

	set_tss64( // 目前所有的栈指针都是7c00
	        0xffff800000007c00,
            0xffff800000007c00,
            0xffff800000007c00,
            0xffff800000007c00,
            0xffff800000007c00,
            0xffff800000007c00,
            0xffff800000007c00,
            0xffff800000007c00,
            0xffff800000007c00,
            0xffff800000007c00
	        );
    sys_vector_init();
	i = 1/0;

	while(1);
}