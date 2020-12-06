#include "memory.h"

void init_memory()
{
    int i,j;
	unsigned long TotalMem = 0 ;
	struct E820 *p = NULL;	

	// 读取内存信息, 只有类型1才是可用的
	color_printk(BLUE,WHITE,"Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
	p = (struct E820 *)0xffff800000007e00;
	for(i = 0;i < 32;i++)
	{
		color_printk(ORANGE,BLACK,"Address:%#018lx\tLength:%#018lx\tType:%#010x\n",p->address,p->length,p->type);
		unsigned long tmp = 0;
		if(p->type == 1) //类型是1的就是可用的,才计算长度
			TotalMem +=  p->length;

		memory_management_struct.e820[i].address += p->address;
		memory_management_struct.e820[i].length	 += p->length;
		memory_management_struct.e820[i].type	 = p->type;
		memory_management_struct.e820_length = i;

		p++;
		if(p->type > 4 || p->length == 0 || p->type < 1)
			break;		
	}

	// 计算可用页的数量
	color_printk(ORANGE,BLACK,"OS Can Used Total RAM:%#018lx\n",TotalMem);
	TotalMem = 0;
	for(i = 0;i <= memory_management_struct.e820_length;i++)
	{
		unsigned long start,end;
		if(memory_management_struct.e820[i].type != 1)
			continue;
		start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address); // 2M对齐
		end   = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT; // 后21位置0，去掉不足2M的部分
		if(end <= start)
			continue;
		TotalMem += (end - start) >> PAGE_2M_SHIFT;
	}
	color_printk(ORANGE,BLACK,"OS Can Used Total 2M PAGEs:%x(=%d)\n",TotalMem,TotalMem); // 到这里TotalMem是页数


	// 页管理相关数据结构初始化
    memory_management_struct.bits_map = (unsigned long *)((memory_management_struct.end_brk + PAGE_4K_SIZE - 1) & PAGE_4K_MASK); // 内核结束后的第一个4k对齐地址存放映射位图
    memory_management_struct.bits_size = TotalMem;
    memory_management_struct.bits_length = (((unsigned long)(memory_management_struct.bits_size) + sizeof(long) * 8 - 1) / 8) & ( ~ (sizeof(long) - 1));
    memset(memory_management_struct.bits_map,0xff,memory_management_struct.bits_length);		// 映射位图全部置1
    color_printk(WHITE,BLACK,"bits_size: %d, bits_leangth: %d\n",memory_management_struct.bits_size,memory_management_struct.bits_length);

    memory_management_struct.pages_struct = (struct Page *)(((unsigned long)memory_management_struct.bits_map + memory_management_struct.bits_length + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    memory_management_struct.pages_size = TotalMem;
    memory_management_struct.pages_length = (TotalMem * sizeof(struct Page) + sizeof(long) - 1) & ( ~ (sizeof(long) - 1));
    memset(memory_management_struct.pages_struct,0x00,memory_management_struct.pages_length);
    color_printk(WHITE,BLACK,"pages_size: %d, pages_leangth: %d\n",memory_management_struct.pages_size,memory_management_struct.pages_length);

    memory_management_struct.zones_struct = (struct Zone *)(((unsigned long)memory_management_struct.pages_struct + memory_management_struct.pages_length + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    memory_management_struct.zones_size   = 0; // 目前无法计算zone的个数, 置0
    memory_management_struct.zones_length = (5 * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));
    memset(memory_management_struct.zones_struct,0x00,memory_management_struct.zones_length);

    // 再次遍历E820来完成各数组成员变量的初始化
    for(i = 0;i <= memory_management_struct.e820_length;i++)
    {
        unsigned long start,end;
        struct Zone * z;
        struct Page * p;
        unsigned long * b;

        if(memory_management_struct.e820[i].type != 1)
            continue;
        start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        end   = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        if(end <= start)
            continue;

        //zone init
        z = memory_management_struct.zones_struct + memory_management_struct.zones_size;
        memory_management_struct.zones_size++;
        z->zone_start_address = start;
        z->zone_end_address = end;
        z->zone_length = end - start;
        z->page_using_count = 0;
        z->page_free_count = (end - start) >> PAGE_2M_SHIFT;
        z->total_pages_link = 0;
        z->attribute = 0;
        z->GMD_struct = &memory_management_struct;
        z->pages_length = (end - start) >> PAGE_2M_SHIFT;
        z->pages_group =  (struct Page *)(memory_management_struct.pages_struct + (start >> PAGE_2M_SHIFT));

        //page init
        p = z->pages_group;
        for(j = 0;j < z->pages_length; j++ , p++)
        {
            p->zone_struct = z;
            p->PHY_address = start + PAGE_2M_SIZE * j;
            p->attribute = 0;
            p->reference_count = 0;
            p->age = 0;
            *(memory_management_struct.bits_map + (p->PHY_address >> PAGE_2M_SHIFT >> 6)) ^= 1UL << (p->PHY_address >> PAGE_2M_SHIFT >> 6);
        }
    }

    //init address 0 to page struct 0; because the memory_management_struct.e820[0].type != 1
    memory_management_struct.pages_struct->zone_struct = memory_management_struct.zones_struct;
    memory_management_struct.pages_struct->PHY_address = 0UL;
    memory_management_struct.pages_struct->attribute = 0;
    memory_management_struct.pages_struct->reference_count = 0;
    memory_management_struct.pages_struct->age = 0;

}