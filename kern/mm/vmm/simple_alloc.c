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

__attribute__((visibility("hidden")))
void* free_start=NULL;
__attribute__((visibility("hidden")))
void* bootstrap_page_start=NULL;

#define BOOT_PAGE_NUM (10)
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
#define GET_SUCC(p) (*((unsigned int *)(p)+WSIZE))
#define PUT_PREV(p,val) (*(unsigned int *)(p) = (val))
#define PUT_SUCC(p,val) (*((unsigned int *)(p)+WSIZE) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
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

static int __get_page() {}

static struct Getpage __getpage = {
	.get_page	__get_page
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
    if(BOOT_PAGE_NUM>count)
        return -1;
    page_ptr=bootstrap_page_start+PAGE_SIZE*count;

    /*deal with edge condition*/
    PUT(page_ptr+2*WSIZE,PACK(0,1));
    PUT(page_ptr+PAGE_SIZE-WSIZE,PACK(0,1));

    /*set up empty list*/
    PUT(page_ptr+3*WSIZE,PACK(PAGE_SIZE-4*WSIZE));
    PUT(page_ptr+PAGE_SIZE-2*WSIZE,PACK(PAGE_SIZE-4*WSIZE));
    add_empty_block(page_ptr+4*WSIZE);
    return 0;
}

/*using first fit policy*/
void* find_fit(uint32_t size)
{
    void* ptr=NULL;
    for(ptr=free_start;ptr!=NULL;ptr=GET_SUCC(ptr))
    {
        if(GET_SIZE(ptr)>=size)
        break;
    }
    return ptr;
}

void place(void* ptr,uint32_t size)
{
    uint32_t all_size=GET_SIZE(ptr);
    if(all_size<size+ALIGN_SIZE)
    {
        PUT(HDRP(ptr),PACK(all_size,1));
        PUT(FTRP(ptr),PACK(all_size,1));
        return;
    }
    PUT(HDRP(ptr),PACK(size,1));
    PUT(FTRP(ptr),PACK(size,1));
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
    uint32_t extendsize=0; /* Amount to extend heap if no fit */
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

int bootstrap_init(void* start)
{
    bootstrap_page_start=start;
    __getpage.get_page=bootstrap_get_page;
    return 0;
}

static inline
int simple_allocator_bootstrap(void *pt, size_t size)
{
    struct simple_allocator allocator_bootstrap = {
        .alloc	= bootstrap_alloc,
        .free	= bootstrap_free,
        .size	= bootstrap_size
    };
}

//int simple_allocator_init(void);

