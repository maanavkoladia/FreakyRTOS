#ifndef OS_HEAP 
#define OS_HEAP 
/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdint.h>
#include <stddef.h>
/* ================================================== */
/*            GLOBAL VARIABLE DECLARATIONS            */
/* ================================================== */
#define MIN_BLOCK_SIZE 8
#define MAX_BLOCK_SIZE 1024
#define SIZE_OF_POOL 1024 * 2 
// struct for holding statistics on the state of the heap

#define MIN_BLOCK_SIZE 8
#define MIN_BLOCK_SHIFT 3 // log2(MIN_BLOCK_SIZE)
#define MAX_BLOCK_SIZE 1024
#define FL_INDEX_CNT 8
#define SL_INDEX_CNT 4
#define SL_INDEX_BITS 2

#define ALLOCATED_BIT  0x1  // LSB, indicates allocation status
#define FREE_BIT (~ALLOCATED_BIT)

#define FLAGS_MASK     0x7  // All lower 3 bits
#define SIZE_MASK      (~FLAGS_MASK)

typedef struct blockHeader_t {
    size_t size;                               // Size of the block (low bits store flags)
    struct blockHeader_t* prevPhysicalBlock;   // For coalescing
    struct blockHeader_t* nextFreePtr;         // Next block in the free list
    struct blockHeader_t* prevFreePtr;         // Prev block in the free list
} blockHeader_t;

// Control structure for the allocator (external, not inside the pool)
typedef struct control_t {
    size_t fl_bitmap;                                 // FL bitmap
    size_t sl_bitmaps[FL_INDEX_CNT];                  // SL bitmap array (one per FL)
    blockHeader_t* blocks[FL_INDEX_CNT][SL_INDEX_CNT]; // Free list heads for each FL/SL
    void* poolStart;                                  // Start of the heap memory region
    size_t poolSize;                                  // Size of the heap memory region
    size_t bytesUsedAllocated;
} control_t;
typedef struct heap_stats {
   size_t size;   // heap size (in bytes)
   size_t used;   // number of bytes used/allocated
   size_t free;   // number of bytes available to allocate
} heap_stats_t;

typedef enum err_t{
    SUCCESS,
    NO_FREE_BLOCKS,
    INIT_FAIL,
    SIZE_TOO_BIG,
    FREE_NULL_ERR
}err_t;

#define Heap_Malloc Malloc
#define Heap_Calloc Calloc
#define Heap_Realloc Realloc
#define Heap_Free Free

/* ================================================== */
/*                 FUNCTION PROTOTYPES                */
/* ================================================== */
/**
 * @details Initialize the Heap
 * @param  none
 * @return always 0 (success)
 * @brief  Initializes/resets the heap to a clean state where no memory
 *         is allocated.
 */
int32_t Heap_Init(void);


/**
 * @details Allocate memory, data not initialized
 * @param  desiredBytes: desired number of bytes to allocate
 * @return void* pointing to the allocated memory or will return NULL
 *         if there isn't sufficient space to satisfy allocation request
 * @brief  Allocate memory
 */
void* Heap_Malloc(size_t desiredBytes);


/**
 * @details Allocate memory, allocated memory is initialized to 0 (zeroed out)
 * @param  desiredBytes: desired number of bytes to allocate
 * @return void* pointing to the allocated memory block or will return NULL
 *         if there isn't sufficient space to satisfy allocation request
 * @brief Zero-allocate memory
 */
void* Heap_Calloc(size_t desiredBytes);


/**
 * @details Reallocate buffer to a new size. The given block may be 
 *          unallocated and its contents copied to a new block
 * @param  oldBlock: pointer to a block
 * @param  desiredBytes: a desired number of bytes for a new block
 * @return void* pointing to the new block or will return NULL
 *         if there is any reason the reallocation can't be completed
 * @brief  Grow/shrink memory
 */
void* Heap_Realloc(void* oldBlock, size_t desiredBytes);


/**
 * @details Return a block to the heap
 * @param  pointer to memory to unallocate
 * @return 0 if everything is ok, non-zero in case of error (e.g. invalid pointer
 *         or trying to unallocate memory that has already been unallocated)
 * @brief  Free memory
 */
int32_t Heap_Free(void* pointer);


/**
 * @details Return the current usage status of the heap
 * @param  reference to a heap_stats_t that returns the current usage of the heap
 * @return 0 in case of success, non-zeror in case of error (e.g. corrupted heap)
 * @brief  Get heap usage
 */
int32_t Heap_Stats(heap_stats_t *stats);

void PrintTlsf_Control(void);

#endif 

