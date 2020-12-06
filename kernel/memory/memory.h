#ifndef LEARNOS_MEMORY_H
#define LEARNOS_MEMORY_H

#include "../print/printk.h"
#include "../lib/lib.h"

// BIOS读出来的内存信息，用结构体来访问
struct E820
{
    unsigned long address;
    unsigned long length;
    unsigned int	type;
}__attribute__((packed));

// 页表,本质上是一个指向页表的地址
typedef struct {unsigned long pml4t;} pml4t_t;
#define	mk_mpl4t(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_mpl4t(mpl4tptr,mpl4tval)	(*(mpl4tptr) = (mpl4tval))
typedef struct {unsigned long pdpt;} pdpt_t;
#define mk_pdpt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdpt(pdptptr,pdptval)	(*(pdptptr) = (pdptval))
typedef struct {unsigned long pdt;} pdt_t;
#define mk_pdt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdt(pdtptr,pdtval)		(*(pdtptr) = (pdtval))
typedef struct {unsigned long pt;} pt_t;
#define mk_pt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pt(ptptr,ptval)		(*(ptptr) = (ptval))
unsigned long * Global_CR3 = NULL;



//页表项个数：每个页表4KB，每项8B，即512项
#define PTRS_PER_PAGE	512

#define PAGE_OFFSET	((unsigned long)0xffff800000000000)

#define PAGE_GDT_SHIFT	39
#define PAGE_1G_SHIFT	30
#define PAGE_2M_SHIFT	21
#define PAGE_4K_SHIFT	12

#define PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT) //1后面21个0
#define PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT) //1后面12个0

#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1)) //后面21个0,前面全是1,用来去掉后面的21位,即2M对齐
#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1)) //后面12个0,前面全是1,用来去掉后面的12位,即4k对齐

//地址加上 2M 之后取前缀, 得到2M页的上界, 本质上就是向后移动到2M对齐的物理地址
#define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

// 当前是统一映射, 所以虚拟到物理直接减去ffff8......即可, 相反, 物理加上ffff8...就得到虚拟
#define Virt_To_Phy(addr)	((unsigned long)(addr) - PAGE_OFFSET)
#define Phy_To_Virt(addr)	((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))

// 
#define Virt_To_2M_Page(kaddr)	(memory_management_struct.pages_struct + (Virt_To_Phy(kaddr) >> PAGE_2M_SHIFT))
#define Phy_to_2M_Page(kaddr)	(memory_management_struct.pages_struct + ((unsigned long)(kaddr) >> PAGE_2M_SHIFT))

#define ZONE_DMA	(1 << 0)
#define ZONE_NORMAL	(1 << 1)
#define ZONE_UNMAPED	(1 << 2)
#define PG_PTable_Maped	(1 << 0)
#define PG_Kernel_Init	(1 << 1)
#define PG_Referenced	(1 << 2)
#define PG_Dirty	(1 << 3)
#define PG_Active	(1 << 4)
#define PG_Up_To_Date	(1 << 5)
#define PG_Device	(1 << 6)
#define PG_Kernel	(1 << 7)
#define PG_K_Share_To_U	(1 << 8)
#define PG_Slab		(1 << 9)

struct Page
{
    struct Zone *	zone_struct; // 指向页所属区域
    unsigned long	PHY_address; // 页的物理地址
    unsigned long	attribute; // 页属性
    unsigned long	reference_count; // 页的引用次数
    unsigned long	age; // 页创建时间
};

struct Zone
{
    struct Page * 	pages_group; // 本区域包含的页数组的指针
    unsigned long	pages_length; // 页数量
    unsigned long	zone_start_address; // 区域起始地址,页对齐
    unsigned long	zone_end_address; // 区域结束地址
    unsigned long	zone_length; // 区域经过对齐之后的长度
    unsigned long	attribute; // 区域的属性
    struct Global_Memory_Descriptor * GMD_struct;
    unsigned long	page_using_count; // 已使用的页数量
    unsigned long	page_free_count; // 空闲页数量
    unsigned long	total_pages_link; // 区域内页被引用的总次数
};

int ZONE_DMA_INDEX	= 0;
int ZONE_NORMAL_INDEX	= 0;	//low 1GB RAM ,was mapped in pagetable
int ZONE_UNMAPED_INDEX	= 0;	//above 1GB RAM,unmapped in pagetable

#define MAX_NR_ZONES	10	//max zone

//全局的内存信息结构体
struct Global_Memory_Descriptor
{
    struct E820 	e820[32];
    unsigned long 	e820_length;

    unsigned long * bits_map; // 页映射位图, 8字节一个,
    unsigned long 	bits_size; // 页数量
    unsigned long   bits_length; // 位图的总字节数

    struct Page *	pages_struct; //指向页数组的指针
    unsigned long	pages_size; // 页数组长度
    unsigned long 	pages_length;

    struct Zone * 	zones_struct; // 区域数组的指针
    unsigned long	zones_size;
    unsigned long 	zones_length;

    unsigned long 	start_code , end_code , end_data , end_brk; // 内核的起始代码段,结束代码段,结束数据段,结束地址

    unsigned long	end_of_struct; // 页数组的结束地址
};
extern struct Global_Memory_Descriptor memory_management_struct;

void init_memory();
unsigned long page_init(struct Page * page,unsigned long flags);

unsigned long page_clean(struct Page * page);

struct Page * alloc_pages(int zone_select,int number,unsigned long page_flags);

#define	flush_tlb_one(addr)	\
	__asm__ __volatile__	("invlpg	(%0)	\n\t"::"r"(addr):"memory")

#define flush_tlb()						\
do								\
{								\
	unsigned long	tmpreg;					\
	__asm__ __volatile__ 	(				\
				"movq	%%cr3,	%0	\n\t"	\
				"movq	%0,	%%cr3	\n\t"	\
				:"=r"(tmpreg)			\
				:				\
				:"memory"			\
				);				\
}while(0)

inline unsigned long * Get_gdt()
{
    unsigned long * tmp;
    __asm__ __volatile__	(
    "movq	%%cr3,	%0	\n\t"
    :"=r"(tmp)
    :
    :"memory"
    );
    return tmp;
}


#endif