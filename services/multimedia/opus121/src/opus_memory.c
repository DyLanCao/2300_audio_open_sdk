#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "plat_types.h"
#include "opus_memory.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "cmsis_nvic.h"

#define RT_USING_HEAP
#define RT_USING_SMALL_MEM
#define rt_size_t uint32_t
#define rt_uint8_t uint8_t
#define rt_uint16_t uint16_t
#define rt_uint32_t uint32_t

#define RT_NULL NULL
#define RT_ASSERT(cond)       { if (!(cond)) { TRACE("%s line: %d\n", __func__, __LINE__); while(1); } }

/* #define RT_MEM_DEBUG */
#define RT_MEM_STATS

#if defined (RT_USING_HEAP) && defined (RT_USING_SMALL_MEM)

#define HEAP_MAGIC 0x1ea0
struct heap_mem
{
    /* magic and used flag */
    uint16_t magic;
    uint16_t used;

    uint32_t next, prev;
};

/** pointer to the heap: for alignment, heap_ptr is now a pointer instead of an array */
static uint8_t *heap_ptr;

/** the last entry, always unused! */
static struct heap_mem *heap_end;

/* RT_ALIGN_SIZE*/
#define RT_ALIGN_SIZE			        32
#define MIN_SIZE 12
#define RT_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))
#define MIN_SIZE_ALIGNED     RT_ALIGN(MIN_SIZE, RT_ALIGN_SIZE)
#define SIZEOF_STRUCT_MEM    RT_ALIGN(sizeof(struct heap_mem), RT_ALIGN_SIZE)

static struct heap_mem *lfree;   /* pointer to the lowest free block */

#if 0
osSemaphoreDef(heap_sem);
osSemaphoreId heap_sem;
#endif

static rt_size_t mem_size_aligned;

#ifdef RT_MEM_STATS
static rt_size_t used_mem, max_mem;
#endif

static void plug_holes(struct heap_mem *mem)
{
    struct heap_mem *nmem;
    struct heap_mem *pmem;

    RT_ASSERT((rt_uint8_t *)mem >= heap_ptr);
    RT_ASSERT((rt_uint8_t *)mem < (rt_uint8_t *)heap_end);
    RT_ASSERT(mem->used == 0);

    /* plug hole forward */
    nmem = (struct heap_mem *)&heap_ptr[mem->next];
    if (mem != nmem &&
        nmem->used == 0 &&
        (rt_uint8_t *)nmem != (rt_uint8_t *)heap_end)
    {
        /* if mem->next is unused and not end of heap_ptr,
         * combine mem and mem->next
         */
        if (lfree == nmem)
        {
            lfree = mem;
        }
        mem->next = nmem->next;
        ((struct heap_mem *)&heap_ptr[nmem->next])->prev = (rt_uint8_t *)mem - heap_ptr;
    }

    /* plug hole backward */
    pmem = (struct heap_mem *)&heap_ptr[mem->prev];
    if (pmem != mem && pmem->used == 0)
    {
        /* if mem->prev is unused, combine mem and mem->prev */
        if (lfree == mem)
        {
            lfree = pmem;
        }
        pmem->next = mem->next;
        ((struct heap_mem *)&heap_ptr[mem->next])->prev = (rt_uint8_t *)pmem - heap_ptr;
    }
}

/**
 * @ingroup SystemInit
 *
 * This function will initialize system heap memory.
 *
 * @param begin_addr the beginning address of system heap memory.
 * @param end_addr the end address of system heap memory.
 */
void rt_system_heap_init(void *begin_addr, void *end_addr)
{
    struct heap_mem *mem;
    rt_uint32_t begin_align = RT_ALIGN((rt_uint32_t)begin_addr, RT_ALIGN_SIZE);
    rt_uint32_t end_align = RT_ALIGN_DOWN((rt_uint32_t)end_addr, RT_ALIGN_SIZE);


    /* alignment addr */
    if ((end_align > (2 * SIZEOF_STRUCT_MEM)) &&
        ((end_align - 2 * SIZEOF_STRUCT_MEM) >= begin_align))
    {
        /* calculate the aligned memory size */
        mem_size_aligned = end_align - begin_align - 2 * SIZEOF_STRUCT_MEM;
    }
    else
    {
        TRACE("mem init, error begin address 0x%x, and end address 0x%x\n",
                   (rt_uint32_t)begin_addr, (rt_uint32_t)end_addr);

        return;
    }

    /* point to begin address of heap */
    heap_ptr = (rt_uint8_t *)begin_align;

    TRACE("mem init, heap begin address 0x%x, size %d\n",
                                (rt_uint32_t)heap_ptr, mem_size_aligned);

    /* initialize the start of the heap */
    mem        = (struct heap_mem *)heap_ptr;
    mem->magic = HEAP_MAGIC;
    mem->next  = mem_size_aligned + SIZEOF_STRUCT_MEM;
    mem->prev  = 0;
    mem->used  = 0;

    /* initialize the end of the heap */
    heap_end        = (struct heap_mem *)&heap_ptr[mem->next];
    heap_end->magic = HEAP_MAGIC;
    heap_end->used  = 1;
    heap_end->next  = mem_size_aligned + SIZEOF_STRUCT_MEM;
    heap_end->prev  = mem_size_aligned + SIZEOF_STRUCT_MEM;

#if 0
    heap_sem = osSemaphoreCreate(osSemaphore(heap_sem), 1);
#endif

    /* initialize the lowest-free pointer to the start of the heap */
    lfree = (struct heap_mem *)heap_ptr;
}

/**
 * @addtogroup MM
 */

/*@{*/

/**
 * Allocate a block of memory with a minimum of 'size' bytes.
 *
 * @param size is the minimum size of the requested block in bytes.
 *
 * @return pointer to allocated memory or NULL if no free memory was found.
 */
void *rt_malloc(rt_size_t size)
{
    rt_size_t ptr, ptr2;
    struct heap_mem *mem, *mem2;
#if 0
    uint32_t lock;
#endif

    if (size == 0)
        return RT_NULL;

#if 0
    if (size != RT_ALIGN(size, RT_ALIGN_SIZE))
        uart_printf("malloc size %d, but align to %d\n",
                                    size, RT_ALIGN(size, RT_ALIGN_SIZE));
    else
        uart_printf("malloc size %d\n", size);
#endif

	TRACE("ask for %d bytes left %d", size, mem_size_aligned);

    /* alignment size */
    size = RT_ALIGN(size, RT_ALIGN_SIZE);

    if (size > mem_size_aligned)
    {
        TRACE("no memory\n");

        return RT_NULL;
    }

    /* every data block must be at least MIN_SIZE_ALIGNED long */
    if (size < MIN_SIZE_ALIGNED)
        size = MIN_SIZE_ALIGNED;

    /* take memory semaphore */
#if 0
    osSemaphoreWait(heap_sem, osWaitForever);
    lock = int_lock();
#endif
    for (ptr = (rt_uint8_t *)lfree - heap_ptr;
         ptr < mem_size_aligned - size;
         ptr = ((struct heap_mem *)&heap_ptr[ptr])->next)
    {
        mem = (struct heap_mem *)&heap_ptr[ptr];

        if ((!mem->used) && (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size)
        {
            /* mem is not used and at least perfect fit is possible:
             * mem->next - (ptr + SIZEOF_STRUCT_MEM) gives us the 'user data size' of mem */

            if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >=
                (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED))
            {
                /* (in addition to the above, we test if another struct heap_mem (SIZEOF_STRUCT_MEM) containing
                 * at least MIN_SIZE_ALIGNED of data also fits in the 'user data space' of 'mem')
                 * -> split large block, create empty remainder,
                 * remainder must be large enough to contain MIN_SIZE_ALIGNED data: if
                 * mem->next - (ptr + (2*SIZEOF_STRUCT_MEM)) == size,
                 * struct heap_mem would fit in but no data between mem2 and mem2->next
                 * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
                 *       region that couldn't hold data, but when mem->next gets freed,
                 *       the 2 regions would be combined, resulting in more free memory
                 */
                ptr2 = ptr + SIZEOF_STRUCT_MEM + size;

                /* create mem2 struct */
                mem2       = (struct heap_mem *)&heap_ptr[ptr2];
                mem2->used = 0;
                mem2->next = mem->next;
                mem2->prev = ptr;

                /* and insert it between mem and mem->next */
                mem->next = ptr2;
                mem->used = 1;

                if (mem2->next != mem_size_aligned + SIZEOF_STRUCT_MEM)
                {
                    ((struct heap_mem *)&heap_ptr[mem2->next])->prev = ptr2;
                }
#ifdef RT_MEM_STATS
                used_mem += (size + SIZEOF_STRUCT_MEM);
                if (max_mem < used_mem)
                    max_mem = used_mem;
#endif
            }
            else
            {
                /* (a mem2 struct does no fit into the user data space of mem and mem->next will always
                 * be used at this point: if not we have 2 unused structs in a row, plug_holes should have
                 * take care of this).
                 * -> near fit or excact fit: do not split, no mem2 creation
                 * also can't move mem->next directly behind mem, since mem->next
                 * will always be used at this point!
                 */
                mem->used = 1;
#ifdef RT_MEM_STATS
                used_mem += mem->next - ((rt_uint8_t*)mem - heap_ptr);
                if (max_mem < used_mem)
                    max_mem = used_mem;
#endif
            }
            /* set memory block magic */
            mem->magic = HEAP_MAGIC;

            if (mem == lfree)
            {
                /* Find next free block after mem and update lowest free pointer */
                while (lfree->used && lfree != heap_end)
                    lfree = (struct heap_mem *)&heap_ptr[lfree->next];

                RT_ASSERT(((lfree == heap_end) || (!lfree->used)));
            }
            
#if 0
            int_unlock(lock);
            osSemaphoreRelease(heap_sem);
#endif
            RT_ASSERT((rt_uint32_t)mem + SIZEOF_STRUCT_MEM + size <= (rt_uint32_t)heap_end);
            RT_ASSERT((rt_uint32_t)((rt_uint8_t *)mem + SIZEOF_STRUCT_MEM) % RT_ALIGN_SIZE == 0);
            RT_ASSERT((((rt_uint32_t)mem) & (RT_ALIGN_SIZE-1)) == 0);

#if 0
            uart_printf("allocate memory at 0x%x, size: %d\n",
                          (rt_uint32_t)((rt_uint8_t *)mem + SIZEOF_STRUCT_MEM),
                          (rt_uint32_t)(mem->next - ((rt_uint8_t *)mem - heap_ptr)));
            uart_printf("rt_malloc mem:0x%x magic:0x%x next:0x%x prv:0x%x used:%d\n",mem, mem->magic, mem->next, mem->prev, used_mem);
#endif

            /* return the memory data except mem struct */
            return (rt_uint8_t *)mem + SIZEOF_STRUCT_MEM;
        }
    }

#if 0
    int_unlock(lock);
    osSemaphoreRelease(heap_sem);
#endif
    return RT_NULL;
}

/**
 * This function will change the previously allocated memory block.
 *
 * @param rmem pointer to memory allocated by rt_malloc
 * @param newsize the required new size
 *
 * @return the changed memory block address
 */
void *rt_realloc(void *rmem, rt_size_t newsize)
{
    rt_size_t size;
    rt_size_t ptr, ptr2;
    struct heap_mem *mem, *mem2;
    void *nmem;

    /* alignment size */
    newsize = RT_ALIGN(newsize, RT_ALIGN_SIZE);
    if (newsize > mem_size_aligned)
    {
        TRACE("realloc: out of memory\n");

        return RT_NULL;
    }

    /* allocate a new memory block */
    if (rmem == RT_NULL)
        return rt_malloc(newsize);

#if 0
    osSemaphoreWait(heap_sem, osWaitForever);
#endif

    if ((rt_uint8_t *)rmem < (rt_uint8_t *)heap_ptr ||
        (rt_uint8_t *)rmem >= (rt_uint8_t *)heap_end)
    {
        /* illegal memory */
#if 0
        osSemaphoreRelease(heap_sem);
#endif

        return rmem;
    }

    mem = (struct heap_mem *)((rt_uint8_t *)rmem - SIZEOF_STRUCT_MEM);

    ptr = (rt_uint8_t *)mem - heap_ptr;
    size = mem->next - ptr - SIZEOF_STRUCT_MEM;
    if (size == newsize)
    {
        /* the size is the same as */
#if 0
        osSemaphoreRelease(heap_sem);
#endif

        return rmem;
    }

    if (newsize + SIZEOF_STRUCT_MEM + MIN_SIZE < size)
    {
        /* split memory block */
#ifdef RT_MEM_STATS
        used_mem -= (size - newsize);
#endif

        ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
        mem2 = (struct heap_mem *)&heap_ptr[ptr2];
        mem2->magic= HEAP_MAGIC;
        mem2->used = 0;
        mem2->next = mem->next;
        mem2->prev = ptr;
        mem->next = ptr2;
        if (mem2->next != mem_size_aligned + SIZEOF_STRUCT_MEM)
        {
            ((struct heap_mem *)&heap_ptr[mem2->next])->prev = ptr2;
        }

        plug_holes(mem2);

#if 0
        osSemaphoreRelease(heap_sem);
#endif

        return rmem;
    }
#if 0
    osSemaphoreRelease(heap_sem);
#endif

    /* expand memory */
    nmem = rt_malloc(newsize);
    if (nmem != RT_NULL) /* check memory */
    {
        memcpy(nmem, rmem, size < newsize ? size : newsize);
        rt_free(rmem);
    }

    return nmem;
}


/**
 * This function will contiguously allocate enough space for count objects
 * that are size bytes of memory each and returns a pointer to the allocated
 * memory.
 *
 * The allocated memory is filled with bytes of value zero.
 *
 * @param count number of objects to allocate
 * @param size size of the objects to allocate
 *
 * @return pointer to allocated memory / NULL pointer if there is an error
 */
void *rt_calloc(rt_size_t count, rt_size_t size)
{
    void *p;
    uint32_t lock;
    /* allocate 'count' objects of size 'size' */
    p = rt_malloc(count * size);

    lock = int_lock();
    /* zero the memory */
    if (p)
        memset(p, 0, count * size);
    int_unlock(lock);
    return p;
}

/**
 * This function will release the previously allocated memory block by
 * rt_malloc. The released memory block is taken back to system heap.
 *
 * @param rmem the address of memory which will be released
 */
void rt_free(void *rmem)
{
    struct heap_mem *mem;

    if (rmem == RT_NULL)
        return;
    RT_ASSERT((((rt_uint32_t)rmem) & (RT_ALIGN_SIZE-1)) == 0);
    RT_ASSERT((rt_uint8_t *)rmem >= (rt_uint8_t *)heap_ptr &&
              (rt_uint8_t *)rmem < (rt_uint8_t *)heap_end);


    if ((rt_uint8_t *)rmem < (rt_uint8_t *)heap_ptr ||
        (rt_uint8_t *)rmem >= (rt_uint8_t *)heap_end)
    {
        TRACE("illegal memory\n");

        return;
    }

    /* Get the corresponding struct heap_mem ... */
    mem = (struct heap_mem *)((rt_uint8_t *)rmem - SIZEOF_STRUCT_MEM);

#if 0
    uart_printf("release memory 0x%x, size: %d used_mem: %d\n",
                  (rt_uint32_t)rmem,
                  (rt_uint32_t)(mem->next - ((rt_uint8_t *)mem - heap_ptr)),
                  used_mem - (mem->next - ((rt_uint8_t *)mem - heap_ptr)));
#endif


    /* protect the heap from concurrent access */
#if 0
    osSemaphoreWait(heap_sem, osWaitForever);
#endif

	TRACE("Free 0x%x", (rt_uint32_t)rmem);

    /* ... which has to be in a used state ... */
    RT_ASSERT(mem->used);
    RT_ASSERT(mem->magic == HEAP_MAGIC);
    /* ... and is now unused. */
    mem->used  = 0;
    mem->magic = 0;

    if (mem < lfree)
    {
        /* the newly freed struct is now the lowest */
        lfree = mem;
    }

#ifdef RT_MEM_STATS
    used_mem -= (mem->next - ((rt_uint8_t*)mem - heap_ptr));
#endif

    /* finally, see if prev or next are free also */
    plug_holes(mem);
#if 0
    osSemaphoreRelease(heap_sem);
#endif
}

#ifdef RT_MEM_STATS
void rt_memory_info(rt_uint32_t *total,
                    rt_uint32_t *used,
                    rt_uint32_t *max_used)
{
    if (total != RT_NULL)
        *total = mem_size_aligned;
    if (used  != RT_NULL)
        *used = used_mem;
    if (max_used != RT_NULL)
        *max_used = max_mem;
}
#endif

#endif
