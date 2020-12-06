#include "print/printk.h"
#include "interrupt/gate.h"
#include "interrupt/trap.h"
#include "memory/memory.h"

// 全局的内存描述符
struct Global_Memory_Descriptor memory_management_struct = {{0},0};

// 链接脚本中定义的内核的段的地址
extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

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

	set_tss64( // 目前所有ist内的栈指针都是7c00，真正用到的是ist1
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

    memory_management_struct.start_code = (unsigned long)& _text;
    memory_management_struct.end_code   = (unsigned long)& _etext;
    memory_management_struct.end_data   = (unsigned long)& _edata;
    memory_management_struct.end_brk    = (unsigned long)& _end; // 内核程序的结束地址

    // 得到可用内存段:00000000_00000000开始的9f000字节，即636k
    // 00000000_00100000开始的7fef0000字节，即2096064k，加起来2047M字节
    color_printk(RED,BLACK,"memory init \n");
	init_memory();

	while(1);
}