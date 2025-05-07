/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "OS_Heap.h"
#include "OS.h"
#include <stddef.h> // For size_t
#include <stdint.h>
#include <stdbool.h>
#include "tlsf.h"

#include "printf.h"
/* ================================================== */
/*                 CONSTANTS/MACROS                   */
/* ================================================== */
#define SIZE_OF_POOL 1024 * 8
/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
#define PD0  (*((volatile uint32_t *)0x40007004))
#define PD1  (*((volatile uint32_t *)0x40007008))
#define PD2  (*((volatile uint32_t *)0x40007010))
#define PD3  (*((volatile uint32_t *)0x40007020))

static uint8_t memPool[SIZE_OF_POOL];
tlsf_t ctrl;
sema4_t heapMutex;
/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */
void* memset0_Align8(void* dest, size_t n);
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
    ctrl = tlsf_create_with_pool(memPool, SIZE_OF_POOL);
    OS_InitSemaphore(&heapMutex, 1);
    return 0;   // replace
}

//******** Heap_Malloc *************** 
// Allocate memory, data not initialized
// input: 
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory or will return NULL
//   if there isn't sufficient space to satisfy allocation request
void* Heap_Malloc(size_t desiredBytes) {
    //int status = StartCritical();
    if(OS_Wait(&heapMutex) == WAIT_REJECTED){
        return NULL;
    }

    void* payLoad = tlsf_malloc(ctrl, desiredBytes); 
    OS_Signal(&heapMutex);

    //EndCritical(status);
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
    //int status = StartCritical();
    OS_Wait(&heapMutex);
    void* payLoad = tlsf_malloc(ctrl, desiredBytes); 
    memset0_Align8(payLoad, desiredBytes);
    OS_Signal(&heapMutex);
    //EndCritical(status);
    return payLoad;
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
    //int status = StartCritical();
    OS_Wait(&heapMutex);
    void* payLoad = tlsf_realloc(ctrl, oldBlock, desiredBytes);
    OS_Signal(&heapMutex);
    //EndCritical(status);
    return payLoad;
}

//******** Heap_Free *************** 
// return a block to the heap
// input: pointer to memory to unallocate
// output: 0 if everything is ok, non-zero in case of error (e.g. invalid pointer
//     or trying to unallocate memory that has already been unallocated
int32_t Heap_Free(void* pointer){
    //int status = StartCritical();
    
    if(OS_Wait(&heapMutex) == WAIT_REJECTED){
        return WAIT_REJECTED;
    }

    tlsf_free(ctrl, pointer);
    OS_Signal(&heapMutex);
    //EndCritical(status);
    return SUCCESS;
}

//******** Heap_Stats *************** 
// return the current status of the heap
// input: reference to a heap_stats_t that returns the current usage of the heap
// output: 0 in case of success, non-zeror in case of error (e.g. corrupted heap)
int32_t Heap_Stats(heap_stats_t *stats){
    stats->used = 0;
    stats->size = tlsf_size(); 
    stats->free = 0;
    return 0;
}

void* memset0_Align8(void* dest, size_t n) {
    uint32_t* ptr = (uint32_t*) dest;
    size_t words = n >> 2;

    for (size_t i = 0; i < words; i++) {
        *ptr++ = 0;
    }

    return dest;
}
