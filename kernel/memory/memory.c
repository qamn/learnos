#include "memory.h"

unsigned long page_init(struct Page *page, unsigned long flags) {
    if (!page->attribute) { // 没有属性
        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |=
                1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
        page->attribute = flags;
        page->reference_count++;
        page->zone_struct->page_using_count++;
        page->zone_struct->page_free_count--;
        page->zone_struct->total_pages_link++;
    } else if ((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U) || (flags & PG_Referenced) ||
               (flags & PG_K_Share_To_U)) {
        page->attribute |= flags;
        page->reference_count++;
        page->zone_struct->total_pages_link++;
    } else {
        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |=
                1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
        page->attribute |= flags;
    }
    return 0;
}

unsigned long page_clean(struct Page *page) {
    if (!page->attribute) {
        page->attribute = 0;
    } else if ((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U)) {
        page->reference_count--;
        page->zone_struct->total_pages_link--;
        if (!page->reference_count) {
            page->attribute = 0;
            page->zone_struct->page_using_count--;
            page->zone_struct->page_free_count++;
        }
    } else {
        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) &= ~(1UL
                << (page->PHY_address >> PAGE_2M_SHIFT) % 64);

        page->attribute = 0;
        page->reference_count = 0;
        page->zone_struct->page_using_count--;
        page->zone_struct->page_free_count++;
        page->zone_struct->total_pages_link--;
    }
    return 0;
}

void init_memory() {
    int i, j;
    unsigned long TotalMem = 0;
    struct E820 *p = NULL;

    // 读取内存信息, 只有类型1才是可用的
//    color_printk(BLUE, WHITE,
//                 "Display Physics Address MAP,Type(1:RAM,  2:ROM or Reserved,  3:ACPI Reclaim Memory,  4:ACPI NVS Memory,  Others:Undefine)\n");
    p = (struct E820 *) 0xffff800000007e00;
    for (i = 0; i < 32; i++) {
//        color_printk(ORANGE, BLACK, "Address:%#018lx\tLength:%#018lx\tType:%#010x\n", p->address, p->length, p->type);
        unsigned long tmp = 0;
        if (p->type == 1) //类型是1的就是可用的,才计算长度
            TotalMem += p->length;

        memory_management_struct.e820[i].address += p->address;
        memory_management_struct.e820[i].length += p->length;
        memory_management_struct.e820[i].type = p->type;
        memory_management_struct.e820_length = i;

        p++;
        if (p->type > 4 || p->length == 0 || p->type < 1)
            break;
    }
    color_printk(ORANGE, BLACK, "OS Can Used Total RAM:%#018lx, e820length:%d\n", TotalMem,
                 memory_management_struct.e820_length);

    // 计算可用页的数量
    TotalMem = 0;
    for (i = 0; i <= memory_management_struct.e820_length; i++) {
        unsigned long start, end;
        if (memory_management_struct.e820[i].type != 1)
            continue;
        start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address); // 2M对齐
        end = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT)
                << PAGE_2M_SHIFT; // 后21位置0，去掉不足2M的部分
        if (end <= start)
            continue;
        TotalMem += (end - start) >> PAGE_2M_SHIFT;
    }
    color_printk(ORANGE, BLACK, "OS Can Used Total 2M PAGEs:%x(=%d)\n", TotalMem, TotalMem); // 到这里TotalMem是页数


    // 页管理相关数据结构初始化
    memory_management_struct.bits_map = (unsigned long *) ((memory_management_struct.end_brk + PAGE_4K_SIZE - 1) &
                                                           PAGE_4K_MASK); // 内核结束后的第一个4k对齐地址存放映射位图
    memory_management_struct.bits_size = TotalMem;
    memory_management_struct.bits_length =
            (((unsigned long) (memory_management_struct.bits_size) + sizeof(long) * 8 - 1) / 8) & (~(sizeof(long) - 1));
    memset(memory_management_struct.bits_map, 0xff, memory_management_struct.bits_length);        // 映射位图全部置1

    memory_management_struct.pages_struct = (struct Page *) (
            ((unsigned long) memory_management_struct.bits_map + memory_management_struct.bits_length + PAGE_4K_SIZE -
             1) & PAGE_4K_MASK);
    memory_management_struct.pages_size = TotalMem;
    memory_management_struct.pages_length = (TotalMem * sizeof(struct Page) + sizeof(long) - 1) & (~(sizeof(long) - 1));
    memset(memory_management_struct.pages_struct, 0x00, memory_management_struct.pages_length);

    memory_management_struct.zones_struct = (struct Zone *) (
            ((unsigned long) memory_management_struct.pages_struct + memory_management_struct.pages_length +
             PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    memory_management_struct.zones_size = 0; // 目前无法计算zone的个数, 置0
    memory_management_struct.zones_length = (5 * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));
    memset(memory_management_struct.zones_struct, 0x00, memory_management_struct.zones_length);

    // 再次遍历E820来完成各数组成员变量的初始化
    for (i = 0; i <= memory_management_struct.e820_length; i++) {
        if (memory_management_struct.e820[i].type != 1)
            continue;

        unsigned long start, end;
        struct Zone *z;
        struct Page *p;
        unsigned long *b;
        start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        end = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT)
                << PAGE_2M_SHIFT;
        if (end <= start)
            continue;

        //zone init
        z = memory_management_struct.zones_struct + memory_management_struct.zones_size; //初始是0
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
        z->pages_group = (struct Page *) (memory_management_struct.pages_struct + (start >> PAGE_2M_SHIFT));

        //page init
        p = z->pages_group;
        for (j = 0; j < z->pages_length; j++, p++) {
            p->zone_struct = z;
            p->PHY_address = start + PAGE_2M_SIZE * j;
            p->attribute = 0;
            p->reference_count = 0;
            p->age = 0;
            *(memory_management_struct.bits_map + ((p->PHY_address >> PAGE_2M_SHIFT) >> 6)) ^=
                    1UL << (p->PHY_address >> PAGE_2M_SHIFT) % 64;
        }
    }

    //init address 0 to page struct 0; because the memory_management_struct.e820[0].type != 1
    memory_management_struct.pages_struct->zone_struct = memory_management_struct.zones_struct;
    memory_management_struct.pages_struct->PHY_address = 0UL;
    memory_management_struct.pages_struct->attribute = 0;
    memory_management_struct.pages_struct->reference_count = 0;
    memory_management_struct.pages_struct->age = 0;

    memory_management_struct.zones_length =
            (memory_management_struct.zones_size * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));
//    color_printk(ORANGE, BLACK, "bits_map:%#018lx,bits_size:%#018lx,bits_length:%#018lx\n",
//                 memory_management_struct.bits_map, memory_management_struct.bits_size,
//                 memory_management_struct.bits_length);
//    color_printk(ORANGE, BLACK, "pages_struct:%#018lx,pages_size:%#018lx,pages_length:%#018lx\n",
//                 memory_management_struct.pages_struct, memory_management_struct.pages_size,
//                 memory_management_struct.pages_length);
//    color_printk(ORANGE, BLACK, "zones_struct:%#018lx,zones_size:%#018lx,zones_length:%#018lx\n",
//                 memory_management_struct.zones_struct, memory_management_struct.zones_size,
//                 memory_management_struct.zones_length);

    ZONE_DMA_INDEX = 0;    //need rewrite in the future
    ZONE_NORMAL_INDEX = 0;    //need rewrite in the future

    for (i = 0; i < memory_management_struct.zones_size; i++)    //need rewrite in the future
    {
        struct Zone *z = memory_management_struct.zones_struct + i;
//        color_printk(ORANGE, BLACK,
//                     "zone_start_address:%#018lx,zone_end_address:%#018lx,zone_length:%#018lx,pages_group:%#018lx,pages_length:%#018lx\n",
//                     z->zone_start_address, z->zone_end_address, z->zone_length, z->pages_group, z->pages_length);
        if (z->zone_start_address == 0x100000000)
            ZONE_UNMAPED_INDEX = i;
    }

    memory_management_struct.end_of_struct = (unsigned long) ((unsigned long) memory_management_struct.zones_struct +
                                                              memory_management_struct.zones_length +
                                                              sizeof(long) * 32) & (~(sizeof(long) -
                                                                                      1));    ////need a blank to separate memory_management_struct
//    color_printk(ORANGE, BLACK,
//                 "start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,end_brk:%#018lx,end_of_struct:%#018lx\n",
//                 memory_management_struct.start_code, memory_management_struct.end_code,
//                 memory_management_struct.end_data, memory_management_struct.end_brk,
//                 memory_management_struct.end_of_struct);

    i = Virt_To_Phy(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT; // 内核占了 i 个页

    for (j = 0; j <= i; j++) {
        page_init(memory_management_struct.pages_struct + j, PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel);
    }

    Global_CR3 = Get_gdt();
    color_printk(INDIGO, BLACK, "Global_CR3\t:%#018lx\n", Global_CR3);
    color_printk(INDIGO, BLACK, "*Global_CR3\t:%#018lx\n", *Phy_To_Virt(Global_CR3) & (~0xff));
    color_printk(INDIGO, BLACK, "**Global_CR3\t:%#018lx\n", *Phy_To_Virt(*Phy_To_Virt(Global_CR3) & (~0xff)) & (~0xff));

    // 这里消除统一映射的页表，这之后只能通过ffff开头的线性地址来访问。
    for (i = 0; i < 10; i++)
        *(Phy_To_Virt(Global_CR3) + i) = 0UL;

    flush_tlb();
}

// 通过找bitmap来返回一个内存页的结构体,其中包括了地址等信息,这就是分配物理页
struct Page *alloc_pages(int zone_select, int number, unsigned long page_flags) {
    int i;
    unsigned long page = 0;

    int zone_start = 0;
    int zone_end = 0;

    switch (zone_select) {
        case ZONE_DMA:
            zone_start = 0;
            zone_end = ZONE_DMA_INDEX;
            break;
        case ZONE_NORMAL:
            zone_start = ZONE_DMA_INDEX;
            zone_end = ZONE_NORMAL_INDEX;
            break;
        case ZONE_UNMAPED:
            zone_start = ZONE_UNMAPED_INDEX;
            zone_end = memory_management_struct.zones_size - 1;
            break;
        default:
            color_printk(RED, BLACK, "alloc_pages error zone_select index\n");
            return NULL;
    }
    for (i = zone_start; i <= zone_end; i++) {
        struct Zone *z;
        unsigned long j;
        unsigned long start, end, length;
        unsigned long tmp;

        if ((memory_management_struct.zones_struct + i)->page_free_count < number) {
            continue;
        }

        z = memory_management_struct.zones_struct + i;
        start = z->zone_start_address >> PAGE_2M_SHIFT;
        end = z->zone_end_address >> PAGE_2M_SHIFT;
        length = z->zone_length >> PAGE_2M_SHIFT;
        tmp = 64 - start % 64;

        for (j = start; j <= end; j += (j % 64) ? tmp : 64) {
            unsigned long *p = memory_management_struct.bits_map + (j >> 6);
            unsigned long shift = j % 64;
            unsigned long k;
            for (k = shift; k < 64 - shift; k++) {
                if (!(((*p >> k) | (*(p + 1) << (64 - k))) &
                      (number == 64 ? 0xffffffffffffffffUL : ((1UL << number) - 1)))) {
                    unsigned long l;
                    page = j + k - 1;
                    for (l = 0; l < number; l++) {
                        struct Page *x = memory_management_struct.pages_struct + page + l;
                        page_init(x, page_flags);
                    }
                    goto find_free_pages;
                }
            }
        }
    }
    return NULL;

    find_free_pages:
    return (struct Page *) (memory_management_struct.pages_struct + page);
}
