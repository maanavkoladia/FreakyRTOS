/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "heap.h"
//#include "OS.h"
#include <stddef.h> // For size_t
#include <stdint.h>

#include "printf.h"

/* ================================================== */
/*                 CONSTANTS/MACROS                   */
/* ================================================== */

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

// Global control structure (single heap version)
control_t tlsf_control;
static uint8_t memPool[SIZE_OF_POOL];

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */
int Init_ControlStruct(void* mem, size_t size);
void GetMapping_ForInsert(size_t size, size_t* fl, size_t* sl);
err_t GetMapping_ForRemove(size_t size, size_t* fl, size_t* sl);
void InsertBlock(blockHeader_t* blk, size_t fl, size_t sl);
blockHeader_t* RemoveFreeBlock(size_t fl, size_t sl);
blockHeader_t* SplitBlock(blockHeader_t* block, size_t desiredSize);
blockHeader_t* CoalesceBlock(blockHeader_t* block);
blockHeader_t* CombineBlocks(blockHeader_t* blk1, blockHeader_t* blk2);
void RemoveBlock(blockHeader_t* blk, size_t fl, size_t sl);

void* memset0_Align8(void* dest, size_t n);
void* memcpy_align8(void* dest, void* src, size_t n);

#define GetActualSize(x)  (((x) & SIZE_MASK) - sizeof(blockHeader_t))
#define CTZ(x) (__builtin_clz(__builtin_bitreverse32(x)))
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_UP_8(x) ALIGN_UP(x, 8)
#define MARK_BLOCK_AS_FREE(blk) (blk->size &= FREE_BIT)
#define MARK_BLOCK_AS_USED(blk)   (blk->size |= ALLOCATED_BIT)

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
//******** Heap_Init *************** 
// Initialize the Heap
// input: none
// output: always 0
// notes: Initializes/resets the heap to a clean state where no memory
//  is allocated.
int32_t Heap_Init(void){
    int out = Init_ControlStruct(memPool, SIZE_OF_POOL);
    //if(out == SUCCESS){
    //    printf("Heap Initialization success\n\r");
    //}else{
    //    printf("Heap Initialization Fail");
    //}
    //heapMutex = (sema4_t*)Malloc(sizeof(sema4_t));
    //OS_InitSemaphore(&heapMutex, 1);
    return 0;   // replace
}

//******** Heap_Malloc *************** 
// Allocate memory, data not initialized
// input: 
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory or will return NULL
//   if there isn't sufficient space to satisfy allocation request
void* Heap_Malloc(size_t desiredBytes) {
    //if(OS_Wait(&heapMutex) == WAIT_REJECTED){
    //    return NULL;
    //}
    if (desiredBytes > MAX_BLOCK_SIZE || desiredBytes < 1) {
        return NULL;
    }

    if (desiredBytes < MIN_BLOCK_SIZE) {
        desiredBytes = MIN_BLOCK_SIZE;
    }

    desiredBytes = ALIGN_UP_8(desiredBytes);

    size_t fl, sl;

    if (GetMapping_ForRemove(desiredBytes, &fl, &sl) != SUCCESS) {
        return NULL;
    }

    blockHeader_t* blk = RemoveFreeBlock(fl, sl);

    blk = SplitBlock(blk, desiredBytes);

    void* payLoad = (void*)((uint8_t*)blk + sizeof(blockHeader_t));

    tlsf_control.bytesUsedAllocated += blk->size & SIZE_MASK;

    return payLoad;
}


//******** Heap_Calloc *************** 
// Allocate memory, data are initialized to 0
// input:
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory block or will return NULL
//   if there isn't sufficient space to satisfy allocation request
//notes: the allocated memory block will be zeroed out
void* Heap_Calloc(size_t desiredBytes) {  
    void* ptr = Heap_Malloc(desiredBytes);
    if (ptr == NULL) {
        return NULL;
    }

    // Zero out the memory
    memset0_Align8(ptr, desiredBytes);

    return ptr;
}


//******** Heap_Realloc *************** 
// Reallocate buffer to a new size
//input: 
//  oldBlock: pointer to a block
//  desiredBytes: a desired number of bytes for a new block
// output: void* pointing to the new block or will return NULL
//   if there is any reason the reallocation can't be completed
// notes: the given block may be unallocated and its contents
//   are copied to a new block if growing/shrinking not possible
void* Heap_Realloc(void* oldBlock, size_t desiredBytes) {
    if (oldBlock == NULL) {
        return Heap_Malloc(desiredBytes); 
    }

    if (desiredBytes > MAX_BLOCK_SIZE || desiredBytes < 1) {
        blockHeader_t* oldblk = (blockHeader_t*)((uint8_t*)oldBlock - sizeof(blockHeader_t));
        Heap_Free(oldBlock);
        return NULL;
    }

    if (desiredBytes < MIN_BLOCK_SIZE) {
        desiredBytes = MIN_BLOCK_SIZE;
    }

    desiredBytes = ALIGN_UP_8(desiredBytes);

    blockHeader_t* blk = (blockHeader_t*)((uint8_t*)oldBlock - sizeof(blockHeader_t));
    size_t oldSize = GetActualSize(blk->size);

    void* ptr;

    if (desiredBytes <= oldSize) {
        blk = SplitBlock(blk, desiredBytes);
        size_t newSize = GetActualSize(blk->size);
        ptr = (void*)((uint8_t*)blk + sizeof(blockHeader_t));

        // Only subtract the size difference
        if(newSize != oldSize){
            tlsf_control.bytesUsedAllocated -= (oldSize - desiredBytes);
        }

        return ptr;
    }

    ptr = Heap_Malloc(desiredBytes);
    if (ptr == NULL) {
        return NULL;
    }

    memcpy_align8(ptr, oldBlock, oldSize);

    Heap_Free(oldBlock);

    return ptr;
}

//******** Heap_Free *************** 
// return a block to the heap
// input: pointer to memory to unallocate
// output: 0 if everything is ok, non-zero in case of error (e.g. invalid pointer
//     or trying to unallocate memory that has already been unallocated
int32_t Heap_Free(void* pointer){
    if(pointer == NULL){
        return FREE_NULL_ERR;
    }

    blockHeader_t* blk = (blockHeader_t*)((uint8_t*)pointer - sizeof(blockHeader_t));
    tlsf_control.bytesUsedAllocated-=(blk->size & SIZE_MASK);
    blk = CoalesceBlock(blk);

    size_t fl, sl; 
    GetMapping_ForInsert(GetActualSize(blk->size), &fl, &sl);
    InsertBlock(blk, fl, sl);
    return SUCCESS;
}

//******** Heap_Stats *************** 
// return the current status of the heap
// input: reference to a heap_stats_t that returns the current usage of the heap
// output: 0 in case of success, non-zeror in case of error (e.g. corrupted heap)
int32_t Heap_Stats(heap_stats_t *stats){
    stats->used = tlsf_control.bytesUsedAllocated;
    stats->size = tlsf_control.poolSize;
    stats->free = tlsf_control.poolSize - tlsf_control.bytesUsedAllocated;
    return 0;
}

// Initialize the allocator's control structure and first block
int Init_ControlStruct(void* memPool, size_t size) {
    if(memPool == NULL || size < MIN_BLOCK_SIZE + sizeof(blockHeader_t)){
        return INIT_FAIL;
    }

    tlsf_control.fl_bitmap = 0;

    for(int i = 0; i < FL_INDEX_CNT; i++){
        tlsf_control.sl_bitmaps[i] = 0;
        for(int j = 0; j < SL_INDEX_CNT; j++){
            tlsf_control.blocks[i][j] = NULL;
        }
    }

    tlsf_control.poolSize = size;
    tlsf_control.poolStart = memPool;
    tlsf_control.bytesUsedAllocated = 0;

    blockHeader_t* firstBlk = (blockHeader_t*)memPool;
    firstBlk->prevPhysicalBlock = NULL; //bc first block
    firstBlk->size = size;
    
    size_t fl, sl;
    GetMapping_ForInsert(GetActualSize(firstBlk->size), &fl, &sl);
    InsertBlock(firstBlk, fl, sl);
    return SUCCESS;
}

// Insert a block into the free list at (fl, sl)
void InsertBlock(blockHeader_t* blk, size_t fl, size_t sl) {
    MARK_BLOCK_AS_FREE(blk);
    blk->prevFreePtr = NULL;
    blk->nextFreePtr = tlsf_control.blocks[fl][sl];
    tlsf_control.blocks[fl][sl] = blk;

    if(blk->nextFreePtr != NULL){
        blk->nextFreePtr->prevFreePtr = blk;
    }

    tlsf_control.fl_bitmap |= (1U << fl);
    tlsf_control.sl_bitmaps[fl] |=  (1U << sl);
}

// Assumes GetMapping() was already called, and a block exists in [fl][sl]
blockHeader_t* RemoveFreeBlock(size_t fl, size_t sl) {
    blockHeader_t* blk = tlsf_control.blocks[fl][sl];

    tlsf_control.blocks[fl][sl] = blk->nextFreePtr;

    // Mark as allocated
    MARK_BLOCK_AS_USED(blk);
    // Clean up pointers
    blk->nextFreePtr = NULL;
    blk->prevFreePtr = NULL;

    blockHeader_t* newHead = tlsf_control.blocks[fl][sl];
    if (newHead != NULL) {
        newHead->prevFreePtr = NULL;
    } else {
        // Free list is now empty; update bitmaps
        tlsf_control.sl_bitmaps[fl] &= ~(1U << sl);

        if (tlsf_control.sl_bitmaps[fl] == 0) {
            tlsf_control.fl_bitmap &= ~(1U << fl);
        }
    }

    return blk;
}

// 'size' is the payload size (excluding block header), already aligned to 8
void GetMapping_ForInsert(size_t size, size_t* fl, size_t* sl) {
    size_t msb_pos = 31 - __builtin_clz(size); // Find the highest set bit (like log2(size))

    size_t fl_idx = msb_pos - MIN_BLOCK_SHIFT; // Normalize FL index to start from MIN_BLOCK_SHIFT

    size_t shift = 0; // Will control how we compute SL index
    size_t sl_idx = 0;

    if (fl_idx >= FL_INDEX_CNT - 1) {
        // Too big to fit normally, clamp to last bucket
        
        *fl = FL_INDEX_CNT - 1;
        *sl = 3;
        return;
    }

    // Special handling for small blocks
    if (size == MIN_BLOCK_SIZE) {
        // Smallest possible block (8 bytes payload)
        // This will land in FL = 0 and SL = 0
        shift = 0;
    } else if (size < SMALL_BLOCK_SIZE) {
        // Blocks from 16 to 31 bytes payload
        // Adjust shift to correctly spread these in SL
        // Because 16-31 should be divided finer
        shift = msb_pos - SL_INDEX_BITS + 1;
        sl_idx = (size >> shift) & (SL_INDEX_CNT - 3);
    } else {
        // Normal shift computation for 32 bytes and up
        shift = msb_pos - SL_INDEX_BITS;
        sl_idx = (size >> shift) & (SL_INDEX_CNT - 1);
    }

    *fl = fl_idx;
    *sl = sl_idx;
}

static bool TryFallbackMappings(size_t size, size_t* fl, size_t* sl) {
    // Special case for 8 bytes
    if (size == 8) {
        blockHeader_t* blk = tlsf_control.blocks[1][0];
        if (blk && GetActualSize(blk->size) >= 8) {
            *fl = 1;
            *sl = 0;
            return true;
        }
    }

    // Special case for 16 bytes
    if (size == 16) {
        blockHeader_t* blk = tlsf_control.blocks[1][1];
        if (blk && GetActualSize(blk->size) >= 16) {
            *fl = 1;
            *sl = 1;
            return true;
        }
    }

    // Special case for 24 bytes
    if (size == 24) {
        blockHeader_t* blk = tlsf_control.blocks[2][0];
        if (blk && GetActualSize(blk->size) >= 24) {
            *fl = 2;
            *sl = 0;
            return true;
        }
    }

    // Special case for 1024 bytes — only allow if it exists directly
    if (size == 1024) {
        blockHeader_t* blk = tlsf_control.blocks[5][0];
        if (blk && GetActualSize(blk->size) >= 1024) {
            *fl = 5;
            *sl = 0;
            return true;
        }
    }

    // Fallback to largest bin: FL=7, SL=3
    blockHeader_t* blk = tlsf_control.blocks[FL_INDEX_CNT - 1][SL_INDEX_CNT - 1];
    if (blk && GetActualSize(blk->size) >= size) {
        *fl = FL_INDEX_CNT - 1;
        *sl = SL_INDEX_CNT - 1;
        return true;
    }

    return false;
}

err_t GetMapping_ForRemove(size_t size, size_t* fl, size_t* sl) {
    size_t msb_pos = 31 - __builtin_clz(size);
    size_t initial_fl_idx = msb_pos - MIN_BLOCK_SHIFT;

    size_t shift = msb_pos - SL_INDEX_BITS;
    size_t initial_sl_idx = (size >> shift) & (SL_INDEX_CNT - 1);

    uint32_t fl_map = tlsf_control.fl_bitmap & (~0U << initial_fl_idx);

    if (fl_map != 0) {
        size_t fl_idx = __builtin_ctz(fl_map);
        uint32_t sl_map = tlsf_control.sl_bitmaps[fl_idx];

        if (fl_idx == initial_fl_idx) {
            sl_map &= (~0U << initial_sl_idx);

            if (sl_map != 0) {
                size_t candidate_sl = __builtin_ctz(sl_map);
                blockHeader_t* blk = tlsf_control.blocks[fl_idx][candidate_sl];
                if (blk && GetActualSize(blk->size) >= size) {
                    *fl = fl_idx;
                    *sl = candidate_sl;
                    return SUCCESS;
                }

                // Try next SL in same FL
                sl_map &= ~(1U << candidate_sl);
                if (sl_map != 0) {
                    *fl = fl_idx;
                    *sl = __builtin_ctz(sl_map);
                    return SUCCESS;
                }
            }

            // No good SL in this FL, try next FL
            fl_map &= (~0U << (fl_idx + 1));
            if (fl_map != 0) {
                fl_idx = __builtin_ctz(fl_map);
                sl_map = tlsf_control.sl_bitmaps[fl_idx];
                if (sl_map != 0) {
                    *fl = fl_idx;
                    *sl = __builtin_ctz(sl_map);
                    return SUCCESS;
                }
            }
        } else {
            // Different FL, just take first SL bit
            if (sl_map != 0) {
                *fl = fl_idx;
                *sl = __builtin_ctz(sl_map);
                return SUCCESS;
            }
        }
    }

    // Nothing found, try special fallback logic
    if (TryFallbackMappings(size, fl, sl)) {
        return SUCCESS;
    }

    return NO_FREE_BLOCKS;
}

blockHeader_t* SplitBlock(blockHeader_t* block, size_t desiredSize) {

    size_t blockSize = block->size & SIZE_MASK; 

    size_t remainingSize = blockSize - desiredSize - sizeof(blockHeader_t);

    if (remainingSize < (MIN_BLOCK_SIZE + sizeof(blockHeader_t))) {
        return block;
    }

    blockHeader_t* freeBlock = (blockHeader_t*)((uint8_t*)block + sizeof(blockHeader_t) + desiredSize);

    freeBlock->size = remainingSize;
    freeBlock->prevPhysicalBlock = block;

    block->size = desiredSize + sizeof(blockHeader_t);
    block->size |= ALLOCATED_BIT;

    size_t fl, sl;
    GetMapping_ForInsert(GetActualSize(freeBlock->size), &fl, &sl);
    InsertBlock(freeBlock, fl, sl);

    return block; // Return the allocated (now split) block
}

blockHeader_t* CoalesceBlock(blockHeader_t* block) {
    blockHeader_t* coalescedBlock = block;

    blockHeader_t* prevBlock = block->prevPhysicalBlock;

    // Coalesce with previous block if it's free
    if (prevBlock != NULL && (prevBlock->size & ALLOCATED_BIT) == 0) {
        coalescedBlock = CombineBlocks(coalescedBlock, prevBlock);
    }

    // Compute next physical block pointer (after coalescedBlock)
    blockHeader_t* nextPhyPtr = (blockHeader_t*)((uint8_t*)coalescedBlock + (coalescedBlock->size & SIZE_MASK));
    void* endOfPool = (void*)((uint8_t*)tlsf_control.poolStart + tlsf_control.poolSize);

    // Check if nextPhyPtr is inside pool bounds before accessing its size
    if ((void*)nextPhyPtr < endOfPool) {
        if ((nextPhyPtr->size & ALLOCATED_BIT) == 0) {
            coalescedBlock = CombineBlocks(coalescedBlock, nextPhyPtr);
        }
    }

    return coalescedBlock;
}

//blk1 not in free lists, blk2 in free lists 
blockHeader_t* CombineBlocks(blockHeader_t* blk1, blockHeader_t* blk2) {
    blockHeader_t* lowerBlock;
    blockHeader_t* upperBlock;

    // Figure out which block is lower in memory (this impacts combining)
    if (blk1 < blk2) {
        lowerBlock = blk1;
        upperBlock = blk2;
    } else {
        lowerBlock = blk2;
        upperBlock = blk1;
    }

    // Remove blk2 from the free list (regardless of address order)
    size_t fl, sl;
    GetMapping_ForInsert(GetActualSize(blk2->size), &fl, &sl);
    RemoveBlock(blk2, fl, sl);

    // Combine sizes: lower block absorbs upper block
    lowerBlock->size = (lowerBlock->size & SIZE_MASK) + (upperBlock->size & SIZE_MASK);

    // Update next block's prevPhysicalBlock pointer (if within pool bounds)
    blockHeader_t* nextBlock = (blockHeader_t*)((uint8_t*)lowerBlock + (lowerBlock->size & SIZE_MASK));
    void* endOfPool = (uint8_t*)tlsf_control.poolStart + tlsf_control.poolSize;

    if ((void*)nextBlock < endOfPool) {
        nextBlock->prevPhysicalBlock = lowerBlock;
    }

    return lowerBlock;
}

void RemoveBlock(blockHeader_t* blk, size_t fl, size_t sl) {
    blockHeader_t* prev = blk->prevFreePtr;
    blockHeader_t* next = blk->nextFreePtr;

    if (prev != NULL) {
        prev->nextFreePtr = next;
    }

    if (next != NULL) {
        next->prevFreePtr = prev;
    }

    if (tlsf_control.blocks[fl][sl] == blk) {
        tlsf_control.blocks[fl][sl] = next;

        if (next == NULL) {
            tlsf_control.sl_bitmaps[fl] &= ~(1U << sl);

            if (tlsf_control.sl_bitmaps[fl] == 0) {
                tlsf_control.fl_bitmap &= ~(1U << fl);
            }
        }
    }

    blk->nextFreePtr = NULL;
    blk->prevFreePtr = NULL;
}

// Example helper to print binary values (bits from MSB to LSB)
void PrintBinary(uint32_t value, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        printf("%c", (value & (1U << i)) ? '1' : '0');
    }
}

void PrintTlsf_Control(void) {
    printf("\n\r===== TLSF CONTROL STATE =====\n\r");

    printf("Pool Start      : %u\n\r", (uint32_t)tlsf_control.poolStart);   // pointer as unsigned int
    printf("Pool Size       : %u bytes\n\r", (uint32_t)tlsf_control.poolSize);
    printf("Bytes Allocated : %u\n\r", (uint32_t)tlsf_control.bytesUsedAllocated);
    printf("-------------------------------------\n\r");

    printf("FL Bitmap (%d bits): ", FL_INDEX_CNT);
    PrintBinary(tlsf_control.fl_bitmap, FL_INDEX_CNT);
    printf("\n\r");

    // Iterate over the FL indices
    for (size_t fl = 0; fl < FL_INDEX_CNT; fl++) {
        if ((tlsf_control.fl_bitmap & (1U << fl)) == 0) {
            continue;  // No free blocks in this FL
        }

        printf("\n\rFL[%d]: SL Bitmap (%d bits): ", (int)fl, SL_INDEX_CNT);
        PrintBinary(tlsf_control.sl_bitmaps[fl], SL_INDEX_CNT);
        printf("\n\r");

        for (size_t sl = 0; sl < SL_INDEX_CNT; sl++) {
            blockHeader_t* blk = tlsf_control.blocks[fl][sl];
            if (blk == NULL) {
                continue;
            }

            printf("  SL[%d]:\n\r", (int)sl);

            int blockCount = 0;
            while (blk != NULL) {
                size_t blockSize = blk->size & SIZE_MASK;
                const char* flagStr = (blk->size & ALLOCATED_BIT) ? "Allocated" : "Free";

                printf("    Block %d: Addr: %u | Size: %u | Flags: %s\n\r",
                       blockCount,
                       (uint32_t)blk,
                       (uint32_t)blockSize,
                       flagStr);

                blk = blk->nextFreePtr;
                blockCount++;
            }
        }
    }

    printf("\n\r=====================================\n\r");
}

//assumign 32 bit system
void* memset0_Align8(void* dest, size_t n) {
    uint32_t* ptr = (uint32_t*) dest;
    size_t words = n >> 2;

    for (size_t i = 0; i < words; i++) {
        *ptr++ = 0;
    }

    return dest;
}

void* memcpy_align8(void* dest, void* src, size_t n){
    uint32_t* dptr = (uint32_t*)dest;
    const uint32_t* sptr = (const uint32_t*)src; // src should be const for memcpy semantics
    size_t words = n >> 2;

    for (size_t i = 0; i < words; i++) {
        *dptr++ = *sptr++;
    }

    return dest;
}

// Memory allocation function for FatFs
void* ff_memalloc (size_t msize) {
    return Malloc(msize);  // Or whatever your allocator function is called
}

// Memory free function for FatFs
void ff_memfree (void* mblock) {
    Free(mblock);
}
