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
void* free_start;
__attribute__((visibility("hidden")))
void* free_end;

#define WSIZE (4)
/*alignment*/
#define ALIGN_SIZE (16)
#define OVERHEAD_SIZE (16)
/* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))


/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {             /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) {       /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {       /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {                                      /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
        GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size,0));    /* Free block header */
    PUT(FTRP(bp), PACK(size,0));    /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0, 1));   /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


int mm_init(void)
{
    void* page_ptr;
    page_ptr=get_page();
    if (page_ptr == NULL)
        return -1;
    /*special handlement for boundry*/
    uint32_t i;
    for(i=0;i<3;i++)
        PUT(page_ptr+i*SIZE,0);
    PUT(page_ptr+i*WSIZE,PACK(0,1));

    return 0;
}

void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

//void *mm_malloc(size_t size)
//{
//    size_t asize;   /* Adjusted block size */
//    size_t extendsize; /* Amount to extend heap if no fit */
//    char *bp;
//
//    /* Ignore spurious requests */
//    if (size == 0)
//        return NULL;
//
//    /* Adjust block size to include overhead and alignment reqs. */
//    if (size <= DSIZE)
//        asize = 2*DSIZE;
//    else
//        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
//
//    /* Search the free list for a fit */
//    if ((bp = find_fit(asize)) != NULL) {
//        place(bp, asize);
//        return bp;
//    }
//
//    /* No fit found. Get more memory and place the block */
//    extendsize = MAX(asize,CHUNKSIZE);
//    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
//        return NULL;
//    place(bp, asize);
//    return bp;
//}


void* find_fit(uint32_t size)
{

}

void* alloc_bootstrap(size_t size, gfp_t flags)
{
    uint32_t asize=0;   /* Adjusted block size */
    uint32_t extendsize=0; /* Amount to extend heap if no fit */
    void *bp=NULL;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    asize=(uint32_t)size+OVERHEAD_SIZE;
    asize=(asize/ALIGN_SIZE+((asize%ALIGN_SIZE)!=0))*ALIGN_SIZE;

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

void free_bootstrap(void *obj)
{

}

static inline
int simple_allocator_bootstrap(void *pt, size_t size)
{
    struct simple_allocator allocator_bootstrap = {
        .alloc	= __simple_alloc,
        .free	= __simple_free,
        .size	= __simple_size
    };
}
int simple_allocator_init(void);

