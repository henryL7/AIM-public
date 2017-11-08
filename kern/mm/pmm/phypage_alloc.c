#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
//#include <aim/sync.h>
#include <aim/mmu.h>
#include <aim/pmm.h>
#include <libc/string.h>
#include <util.h>
#include <aim/vmm.h>
#include <aim/console.h>

#define KERN_MAX_SIZE ((uint32_t)0x70000000)
#define PAGE_KIND_NUM (11)
#define MB_SIZE (1<<20)
#define MAX_PAGE_SIZE (1<<22)
#define PAGE_NO(x) ((uint32_t)x/PAGE_SIZE)
#define PAGE_SPLIT(s,a) ((uint32_t)a+(uint32_t)s/2)

// information for physical page
struct phy_page{
    uint32_t count;   // number of reference
    void* lru; // pointer in buddy system
    gfp_t flags;
};

__attribute__((visibility("hidden")))
struct phy_page phy_mem_map[MB_SIZE];

struct buddy_page{
    void* paddr;
    struct buddy_page* prev;
    struct buddy_page* succ;
};

struct page_head{
    struct buddy_page* first;
    uint32_t order;
    uint32_t size;
};

__attribute__((visibility("hidden")))
struct page_system{
    struct page_head plists[PAGE_KIND_NUM]; 
}pagesystem;

//inline void* buddy_split(uint32_t size,void* paddr)
//{
//    return (void*)((uint32_t)paddr+size/2));
//}

void page_mark(void* paddr)
{
    phy_mem_map[PAGE_NO(paddr)].count+=1;
    return;
}

static void* compute_buddy(void*paddr,uint32_t order )
{
    uint32_t mask=1;
    return (void*)(((uint32_t)paddr)^(mask<<(order+12)));
}

// remove free block
void buddy_remove(void* paddr)
{
    struct buddy_page* ptr=phy_mem_map[PAGE_NO(paddr)].lru;
    if(ptr->succ!=NULL)
        ptr->succ->prev=ptr->prev;
    if(ptr->prev!=NULL)
        ptr->prev->succ=ptr->succ;
    kfree(ptr);
    return;
}

// add free block
void buddy_add(struct page_head* head,void* paddr)
{
    uint32_t mask=1;
    void* buddy=(void*)(((uint32_t)paddr)^(mask<<(head->order+12)));
    if(phy_mem_map[PAGE_NO(buddy)].count==0&&head->order<PAGE_KIND_NUM-1)
    {
        phy_mem_map[PAGE_NO(buddy)].count=1;
        buddy_remove(buddy);
        return buddy_add(&pagesystem.plists[head->order+1],min2(paddr,buddy));
    }
    struct buddy_page* np=kmalloc(sizeof(struct buddy_page),0);
    if(np==NULL)return;
    np->paddr=paddr;
    if(head->first!=NULL)
        head->first->prev=np;
    np->succ=head->first;
    np->prev=NULL;
    head->first=np;
    phy_mem_map[PAGE_NO(paddr)].count=0;
    phy_mem_map[PAGE_NO(paddr)].flags=0;
    phy_mem_map[PAGE_NO(paddr)].lru=np;
    return;
}

// get free block
int buddy_get(struct page_head *head,struct pages* pages)
{
    if(head->first==NULL)
    {
        if(head->order==PAGE_KIND_NUM-1)
            return -1;
        return buddy_get(&pagesystem.plists[head->order+1],pages);
    }
    phy_mem_map[PAGE_NO(head->first->paddr)].count+=1;
    phy_mem_map[PAGE_NO(head->first->paddr)].flags=pages->flags;
    pages->paddr=head->first->paddr;
    struct buddy_page* old=head->first;
    head->first=head->first->succ;
    if(head->first!=NULL)
        head->first->prev=NULL;
    if(head->size!=pages->size)
        buddy_add(&pagesystem.plists[head->order-1],PAGE_SPLIT(head->size,old->paddr));
    kfree(old);
    return 0;
}

uint32_t log2(uint32_t x)
{
    uint32_t i=0;
    while(x>>i)
    {
        i++;
    }
    return i-13;
}

int buddy_alloc(struct pages *pages)
{
    if(pages->size>MAX_PAGE_SIZE)
        return -1;
    uint32_t order=log2(pages->size);
    return buddy_get(&pagesystem.plists[order],pages);
}

void buddy_free(struct pages *pages)
{
    if(phy_mem_map[PAGE_NO(pages->paddr)].count>1)
        return;
    uint32_t order=log2(pages->size);
    return buddy_add(&pagesystem.plists[order],pages->paddr);
}
//addr_t buddy_get_free(void);

addr_t dummy_get_free()
{
    return 0;
}

int page_allocator_init(void)
{
    uint32_t mem_size=(uint32_t)get_mem_size();
    uint32_t kern_size=KERN_MAX_SIZE<mem_size?KERN_MAX_SIZE:mem_size;
    uint32_t pagenum=0;
    /*compute kernel code ending position, see arch_mmu.c*/
    extern char page_table_start[];
	uint32_t page_table_align=(uint32_t)page_table_start;
	page_table_align+=KSTACKSIZE;
    page_table_align=(page_table_align/PAGE_SIZE)*PAGE_SIZE+(page_table_align%PAGE_SIZE!=0)*PAGE_SIZE;
    uint32_t page_start=page_table_align+PAGE_SIZE;
    pagenum=(kern_size-page_start)/PAGE_SIZE;
    uint32_t wholenum=KERN_MAX_SIZE/PAGE_SIZE;
    for(uint32_t i=0;i<wholenum;i++)
    {
        phy_mem_map[i].count=1;   
        phy_mem_map[i].lru=NULL;
        phy_mem_map[i].flags=0;
    }
    for(uint32_t i=0;i<PAGE_KIND_NUM-1;i++)
    {
        pagesystem.plists[i].order=i;
        pagesystem.plists[i].size=PAGE_SIZE*(1<<i);
        pagesystem.plists[i].first=NULL;
    }
    struct page_allocator buddy_allocator = {
        .alloc		= buddy_alloc,
        .free		= buddy_free,
        .get_free	= dummy_get_free
    };
    set_page_allocator(&buddy_allocator);
    // one page should do the job
    buddy_add(&pagesystem.plists[0],(void*)page_start);
    return 0;
}

int page_allocator_move(struct simple_allocator *old)
{
    return 0;  // nothing to do
}

void add_memory_pages(void)
{
    uint32_t mem_size=(uint32_t)get_mem_size();
    uint32_t kern_size=KERN_MAX_SIZE<mem_size?KERN_MAX_SIZE:mem_size;
    uint32_t pagenum=0;
    /*compute kernel code ending position, see arch_mmu.c*/
    extern char page_table_start[];
	uint32_t page_table_align=(uint32_t)page_table_start;
	page_table_align+=KSTACKSIZE;
    page_table_align=(page_table_align/PAGE_SIZE)*PAGE_SIZE+(page_table_align%PAGE_SIZE!=0)*PAGE_SIZE;
    uint32_t page_start=page_table_align+2*PAGE_SIZE;
    pagenum=(kern_size-page_start)/PAGE_SIZE;
    page_start+=PAGE_SIZE;
    kprintf("pagenum: %d\n",pagenum);
    for(uint32_t i=0;i<pagenum-1;i++)
    {
        // add all the avaliable pages
        buddy_add(&pagesystem.plists[0],(void*)page_start);
        page_start+=PAGE_SIZE;
        if(i%1000==0)
        kprintf("no : %d, page_addr: %x\n",i,page_start);
        if(i==479213)
        kprintf("no : %d, page_addr: %x\n",i,page_start);
    }
    return ;
}

