#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
//#include <aim/sync.h>
#include <aim/vmm.h>
#include <aim/mmu.h>	/* PAGE_SIZE */
#include <aim/panic.h>
#include <libc/string.h>
#include <aim/pmm.h>

__attribute__((visibility("hidden")))
void* free_start=NULL;
__attribute__((visibility("hidden")))
void* bootstrap_page_start=NULL;
__attribute__((visibility("hidden")))
uint32_t bootpage_num=0;

#define WSIZE (4)
#define DSIZE (8)
/*alignment*/
#define ALIGN_SIZE (16)
#define OVERHEAD_SIZE (16)

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read and write the prev and succ pointer*/
#define GET_PREV(p) (*(unsigned int *)(p))
#define GET_SUCC(p) (*((unsigned int *)(p+WSIZE)))
#define PUT_PREV(p,val) (*(unsigned int *)(p) = (val))
#define PUT_SUCC(p,val) (*((unsigned int *)(p+WSIZE)) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~(0x7))
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((void *)(bp) - WSIZE)
#define FTRP(bp) ((void *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((void *)(bp) + GET_SIZE(((void *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((void *)(bp) - GET_SIZE(((void *)(bp) - DSIZE)))

/*wrapper for two diffrent 'get_page' function*/
struct Getpage{
    int (*get_page)(void);
};

static int __get_page() {return 0;}

static struct Getpage __getpage = {
	.get_page=__get_page
};

/*using LIFO policy*/
void add_empty_block(void *ptr)
{
    if(ptr==NULL)
        return;
    if(free_start!=NULL)
        PUT_PREV(free_start,ptr);
    PUT_SUCC(ptr,free_start);
    PUT_PREV(ptr,NULL);
    free_start=ptr;
    return;
}

void delete_empty_block(void *ptr)
{
    if(ptr==NULL)
        return;
    if(ptr==free_start)
    {
        void* succ_ptr=GET_SUCC(ptr);
        free_start=succ_ptr;
        if(free_start!=NULL)
            PUT_PREV(free_start,NULL);
        return;
    }
    void* succ_ptr=GET_SUCC(ptr);
    void* prev_ptr=GET_PREV(ptr);
    if(succ_ptr!=NULL)
        PUT_PREV(succ_ptr,prev_ptr);
    if(prev_ptr!=NULL)
        PUT_SUCC(prev_ptr,succ_ptr);
}

void *coalesce(void *obj)
{
    uint32_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(obj)));
    uint32_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(obj)));
    uint32_t size = GET_SIZE(HDRP(obj));

    if (prev_alloc && next_alloc) {             /* Case 1 */
        add_empty_block(obj);
    }

    else if (prev_alloc && !next_alloc) {       /* Case 2 */
        void* next_ptr=NEXT_BLKP(obj);
        size += GET_SIZE(HDRP(next_ptr));
        PUT(HDRP(obj), PACK(size, 0));
        PUT(FTRP(obj), PACK(size,0));
        delete_empty_block(next_ptr);
        add_empty_block(obj);
    }

    else if (!prev_alloc && next_alloc) {       /* Case 3 */
        void* prev_ptr=PREV_BLKP(obj);
        size += GET_SIZE(HDRP(PREV_BLKP(obj)));
        PUT(FTRP(obj), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(obj)), PACK(size, 0));
        obj = PREV_BLKP(obj);
        delete_empty_block(prev_ptr);
        add_empty_block(obj);
    }
    else {                                      /* Case 4 */
        void* next_ptr=NEXT_BLKP(obj);
        void* prev_ptr=PREV_BLKP(obj);
        size += GET_SIZE(HDRP(PREV_BLKP(obj)))+GET_SIZE(FTRP(NEXT_BLKP(obj)));
        PUT(HDRP(PREV_BLKP(obj)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(obj)), PACK(size, 0));
        obj = PREV_BLKP(obj);
        delete_empty_block(next_ptr);
        delete_empty_block(prev_ptr);
        add_empty_block(obj);
    }
    return obj;
}

int bootstrap_get_page()
{
    void* page_ptr=NULL;
    static uint32_t count=0;
    if(bootpage_num<count)
        return -1;
    page_ptr=bootstrap_page_start+PAGE_SIZE*count;
    count+=1;
    /*deal with edge condition*/
    PUT(page_ptr+1*WSIZE,PACK(8,1));
    PUT(page_ptr+2*WSIZE,PACK(8,1));
    PUT(page_ptr+PAGE_SIZE-WSIZE,PACK(0,1));

    /*set up empty list*/
    PUT(page_ptr+3*WSIZE,PACK(PAGE_SIZE-4*WSIZE,0));
    PUT(page_ptr+PAGE_SIZE-2*WSIZE,PACK(PAGE_SIZE-4*WSIZE,0));
    add_empty_block(page_ptr+4*WSIZE);
    return 0;
}

/*using first fit policy*/
<<<<<<< HEAD
__attribute__( ( noinline ) )
=======
>>>>>>> 15c64576b5f25b850eba8213c55eb7bfb6487e1c
void* find_fit(uint32_t size) 
{
    void* ptr=NULL;
    for(ptr=free_start;ptr!=NULL;ptr=GET_SUCC(ptr))
    {
        if(GET_SIZE(HDRP(ptr))>=size)
        break;
    }
    return ptr;
}

void place(void* ptr,uint32_t size)
{
    uint32_t all_size=GET_SIZE(HDRP(ptr));
    if(all_size<size+ALIGN_SIZE)
    {
        PUT(HDRP(ptr),PACK(all_size,1));
        PUT(FTRP(ptr),PACK(all_size,1));
        delete_empty_block(ptr);
        return;
    }
    PUT(HDRP(ptr),PACK(size,1));
    PUT(FTRP(ptr),PACK(size,1));
    delete_empty_block(ptr);
    /*divide block*/
    ptr=NEXT_BLKP(ptr);
    PUT(HDRP(ptr),PACK(all_size-size,0));
    PUT(FTRP(ptr),PACK(all_size-size,0));
    coalesce(ptr);
    return;
}

void* bootstrap_alloc(size_t size, gfp_t flags)
{
    uint32_t asize=0;   /* Adjusted block size */
    void *obj=NULL;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    asize=(uint32_t)size+OVERHEAD_SIZE;
    asize=(asize/ALIGN_SIZE+((asize%ALIGN_SIZE)!=0))*ALIGN_SIZE;

    /* Search the free list for a fit */
    if ((obj = find_fit(asize)) != NULL) {
        place(obj, asize);
        return obj;
    }

    /* No fit found. Get more memory and place the block */
    int result=0;
    result=__getpage.get_page();
    if(result==-1)
        return NULL;
    if ((obj = find_fit(asize)) != NULL) {
        place(obj, asize);
        return obj;
    }
    return NULL;
}

void bootstrap_free(void *obj)
{
    size_t size = GET_SIZE(HDRP(obj));

    PUT(HDRP(obj), PACK(size, 0));
    PUT(FTRP(obj), PACK(size, 0));
    coalesce(obj);
    return;
}

size_t bootstrap_size(void *obj)
{
    if(obj==NULL)
        return 0;
    return (size_t)GET_SIZE(HDRP(obj));
}

/*void* get_bootpage_start(size_t size)
{
    uint32_t page_num=0;
    page_num=((uint32_t)size/PAGE_SIZE+((uint32_t)size%PAGE_SIZE!=0))*PAGE_SIZE;
    uint32_t stack_poi,astack_poi;
    __asm__ __volatile__ (
        "movl %%esp,%%eax;"
        :"=a"(stack_poi)::"memory"
    );
    astack_poi=stack_poi;
    if(stack_poi%PAGE_SIZE)
        astack_poi=((uint32_t)(stack_poi>>12)<<12)+PAGE_SIZE;
    uint32_t page_start=0;
    page_start=astack_poi+page_num*PAGE_SIZE;
    __asm__ __volatile__ ("subl %%eax,%%esp;"::"a"(bootstrap_page_start-stack_poi):"memory");
    return (void*)page_start;
}*/

int simple_allocator_bootstrap(void *pt, size_t size)
{
    bootpage_num=((uint32_t)size/PAGE_SIZE);
    bootstrap_page_start=pt;
    __getpage.get_page=bootstrap_get_page;
    struct simple_allocator allocator_bootstrap = {
        .alloc	= bootstrap_alloc,
        .free	= bootstrap_free,
        .size	= bootstrap_size
    };
    set_simple_allocator(&allocator_bootstrap);
    return 0;
}

int init_get_page()
{
    void* page_ptr=NULL;
    addr_t result=pgalloc();
    if(result==-1)
        return -1;
    page_ptr=pa2kva((void*)result);
    /*deal with edge condition*/
    PUT(page_ptr+1*WSIZE,PACK(8,1));
    PUT(page_ptr+2*WSIZE,PACK(8,1));
    PUT(page_ptr+PAGE_SIZE-WSIZE,PACK(0,1));

    /*set up empty list*/
    PUT(page_ptr+3*WSIZE,PACK(PAGE_SIZE-4*WSIZE,0));
    PUT(page_ptr+PAGE_SIZE-2*WSIZE,PACK(PAGE_SIZE-4*WSIZE,0));
    add_empty_block(page_ptr+4*WSIZE);
    return 0;
}

void change_start()
{
    void* page_ptr=NULL;
    addr_t result=pgalloc();
    page_ptr=pa2kva((void*)result);
    /*deal with edge condition*/
    PUT(page_ptr+1*WSIZE,PACK(8,1));
    PUT(page_ptr+2*WSIZE,PACK(8,1));
    PUT(page_ptr+PAGE_SIZE-WSIZE,PACK(0,1));

    /*set up empty list*/
    PUT(page_ptr+3*WSIZE,PACK(PAGE_SIZE-4*WSIZE,0));
    PUT(page_ptr+PAGE_SIZE-2*WSIZE,PACK(PAGE_SIZE-4*WSIZE,0));
    /*discard the page allocated by bootstrap allocator*/
    free_start=NULL;
    add_empty_block(page_ptr+4*WSIZE);
    return;
}

int simple_allocator_init(void)
{
    __getpage.get_page=init_get_page;
    
    struct simple_allocator allocator_init = {
        .alloc	= bootstrap_alloc,
        .free	= bootstrap_free,
        .size	= bootstrap_size
    };
    set_simple_allocator(&allocator_init);
    change_start();
    return 0;
}


