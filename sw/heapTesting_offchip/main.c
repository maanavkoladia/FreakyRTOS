#include "unity.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// Include your heap allocator header
#include "heap.h"
#include "unity_internals.h"

// Error codes expected from your allocator
#define SUCCESS             0
#define MAX_BLOCK_SIZE      1024  // Example, adjust if needed

// Set up / tear down for Unity
void setUp(void) {
    Heap_Init();
}

void tearDown(void) {
    // No cleanup needed
}

/* ============================ */
/*         TEST CASES           */
/* ============================ */

// Test 0: Allocate and immediately free 1000 times
void test0_Malloc(void){
    int* p;
    for(int i = 0; i < 1000; i++){
        p = (int*) Malloc(sizeof(int));
        TEST_ASSERT_NOT_NULL(p);
        *p = 10;
        Free(p);
    }

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// Test 1: Allocate and free one block
void test1_Malloc(void){
    void* ptr = Malloc(438);
    TEST_ASSERT_NOT_NULL(ptr);
    Free(ptr);

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// Test 2: Allocate 3 blocks, free them in different order, observe coalescing
void test2_Malloc(void) {
    void* p1 = Malloc(64);
    void* p2 = Malloc(64);
    void* p3 = Malloc(64);

    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);

    // Free middle block
    Free(p2);

    // Free first block
    Free(p1);

    // Free third block
    Free(p3);

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// Test 3: Allocate 50 blocks with increasing sizes, then free them
void test3_Malloc(void){
    void* ptrs[50];

    for(int i = 1; i < 50; i++){
        ptrs[i] = Malloc(i);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    for(int i = 1; i < 50; i++){
        Free(ptrs[i]);
    }

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// Test 4: Allocate varying sizes, then free them in reverse order
void test4_Stress_AllocFree_VaryingSizes(void) {
    const int max_blocks = 100;
    void* ptrs[max_blocks];
    int allocated_blocks = 0;

    for (int i = 0; i < max_blocks; i++) {
        size_t size = (i + 1) * 8;

        ptrs[i] = Malloc(size);

        if (ptrs[i] == NULL) {
            // Out of memory: stop allocating more
            break;
        }

        allocated_blocks++;
    }

    // Free all successfully allocated blocks in reverse order
    for (int i = allocated_blocks - 1; i >= 0; i--) {
        Free(ptrs[i]);
    }

    // Verify the heap is fully freed
    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// Test 5: Repeatedly fill and empty the heap with small allocations
void test5_Stress_RepeatedFillAndEmpty(void) {
    const int cycles = 100;
    const int blocks_per_cycle = 50;
    void* ptrs[blocks_per_cycle];

    for (int cycle = 0; cycle < cycles; cycle++) {
        for (int i = 0; i < blocks_per_cycle; i++) {
            ptrs[i] = Malloc(16);
            TEST_ASSERT_NOT_NULL(ptrs[i]);
        }

        for (int i = 0; i < blocks_per_cycle; i++) {
            Free(ptrs[i]);
        }

        heap_stats_t stats;
        Heap_Stats(&stats);
        TEST_ASSERT_EQUAL_UINT(0, stats.used);
    }
}

// Test 6: Allocate until the heap is full, then free everything
void test6_Stress_AllocateUntilFull(void) {
    const int max_blocks = 1024;
    void* ptrs[max_blocks];
    int count = 0;

    while (count < max_blocks) {
        ptrs[count] = Malloc(32);
        if (!ptrs[count]) {
            break;  // Out of memory
        }
        count++;
    }

    TEST_ASSERT_GREATER_THAN(0, count);

    for (int i = 0; i < count; i++) {
        Free(ptrs[i]);
    }

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// Test 7: Fragmentation and coalescing by freeing every other block
void test7_Stress_Fragmentation(void) {
    const int max_blocks = 20;
    void* ptrs[max_blocks];

    for (int i = 0; i < max_blocks; i++) {
        size_t size = (i % 4 + 1) * 16;
        ptrs[i] = Malloc(size);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    // Free every other block
    for (int i = 0; i < max_blocks; i += 2) {
        Free(ptrs[i]);
    }

    // Free remaining blocks to force coalescing
    for (int i = 1; i < max_blocks; i += 2) {
        Free(ptrs[i]);
    }

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}


#define TEST8_ALLOC_SIZE (sizeof(uint32_t) + 3)
#define PtrArr_Size (SIZE_OF_POOL / (8 + sizeof(blockHeader_t)))
#define SECOND_NUM_OFFSET 101010

void* ptrs[PtrArr_Size];
void Test8_MaxingOutMalloc_ThenComparingData(void){

    printf("ALLOC SIZE: %d\n\r", TEST8_ALLOC_SIZE);

    PrintTlsf_Control();
    int i;

    for(i = 0; i < PtrArr_Size; i++){
        uint32_t* p = (uint32_t*)Malloc(TEST8_ALLOC_SIZE);
        
        //TEST_ASSERT_NOT_NULL(p);
        if(p == NULL){
            printf("got a null ptr too early\n");
            break; // or return if you want to bail early
        }else{
            //printf("%d allocations so far\n\r", i);
        }

        ptrs[i] = (void*)p;

        *p++ = i;
        *p = i + SECOND_NUM_OFFSET;

        for(int j = 0; j <= i; j++){
            uint32_t* check_ptr = (uint32_t*)ptrs[j];
            TEST_ASSERT_EQUAL_UINT(j, *check_ptr++);
            TEST_ASSERT_EQUAL_UINT(j + SECOND_NUM_OFFSET, *check_ptr);
        }
    }

    void* p = Malloc(1);
    TEST_ASSERT_NULL(p);

    PrintTlsf_Control();

    for(int k = 0; k < i; k++){
        Free(ptrs[k]);
    }

    PrintTlsf_Control();

}
/* ============================ */
/*        EDGE CASE TESTS       */
/* ============================ */

// Malloc 0 bytes returns NULL
void test_MallocZeroSize_ShouldReturnNULL(void) {
    void* p = Malloc(0);
    TEST_ASSERT_NULL(p);
}

// Malloc more than MAX_BLOCK_SIZE returns NULL
void test_MallocMoreThanMaxSize(void) {
    void* p = Malloc(MAX_BLOCK_SIZE + 1);
    TEST_ASSERT_NULL(p);
}

// Free NULL pointer returns FREE_NULL_ERR
void test_FreeNULLPtr(void) {
    int32_t out = Free(NULL);
    TEST_ASSERT_EQUAL_UINT(FREE_NULL_ERR, out);
}

/* ============================ */
/*         CALLOC TESTS         */
/* ============================ */

// Test 1: Calloc returns NULL for zero size
void test_CallocZeroSize_ShouldReturnNULL(void) {
    void* p = Calloc(0);
    TEST_ASSERT_NULL(p);
}

// Test 2: Calloc returns zero-initialized memory (single allocation)
void test_Calloc_SingleAllocation_ZeroInitialized(void) {
    size_t size = 64;
    uint8_t* p = (uint8_t*)Calloc(size);
    TEST_ASSERT_NOT_NULL(p);

    // Verify memory is zeroed
    for (size_t i = 0; i < size; i++) {
        TEST_ASSERT_EQUAL_UINT8(0, p[i]);
    }

    Free(p);

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// Test 3: Calloc multiple blocks, verify zero init and freeing
void test_Calloc_MultipleAllocations_ZeroInitialized(void) {
    const int num_blocks = 10;
    const int block_size = 32;
    void* ptrs[num_blocks];

    for (int i = 0; i < num_blocks; i++) {
        ptrs[i] = Calloc(block_size);
        TEST_ASSERT_NOT_NULL(ptrs[i]);

        uint8_t* p = (uint8_t*)ptrs[i];
        for (int j = 0; j < block_size; j++) {
            TEST_ASSERT_EQUAL_UINT8(0, p[j]);
        }
    }

    for (int i = 0; i < num_blocks; i++) {
        Free(ptrs[i]);
    }

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// Optional Stress Test: Allocate/free many Calloc blocks
void test_Calloc_Stress(void) {
    const int cycles = 50;
    const int block_size = 128;
    void* ptrs[cycles];

    for (int i = 0; i < cycles; i++) {
        ptrs[i] = Calloc(block_size);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    for (int i = 0; i < cycles; i++) {
        Free(ptrs[i]);
    }

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

/* ============================ */
/*        REALLOC TESTS         */
/* ============================ */

// ✅ Test 1: Realloc NULL pointer behaves like Malloc()
void test_Realloc_NULL_Pointer_ShouldAllocate(void) {
    size_t size = 64;
    uint8_t* ptr = (uint8_t*)Realloc(NULL, size);
    TEST_ASSERT_NOT_NULL(ptr);

    Free(ptr);
    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// ✅ Test 2: Realloc with size 0 frees the block and returns NULL
void test_Realloc_SizeZero_ShouldFreeBlock(void) {
    uint8_t* ptr = (uint8_t*)Malloc(64);
    TEST_ASSERT_NOT_NULL(ptr);

    ptr = (uint8_t*)Realloc(ptr, 0);
    TEST_ASSERT_NULL(ptr);

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// ✅ Test 3: Realloc Shrinks the block and preserves data
void test_Realloc_ShrinkBlock_PreservesData(void) {
    uint8_t* ptr = (uint8_t*)Malloc(64);
    TEST_ASSERT_NOT_NULL(ptr);

    // Fill with test data
    for (int i = 0; i < 64; i++) {
        ptr[i] = (uint8_t)i;
    }

    // Shrink the block
    uint8_t* ptr_shrink = (uint8_t*)Realloc(ptr, 32);
    TEST_ASSERT_NOT_NULL(ptr_shrink);

    // Verify the first 32 bytes are still correct
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL_UINT8(i, ptr_shrink[i]);
    }

    Free(ptr_shrink);
    heap_stats_t stats;
    Heap_Stats(&stats);
    //PrintTlsf_Control();
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// ✅ Test 4: Realloc Grows the block and preserves old data
void test_Realloc_GrowBlock_PreservesData(void) {
    uint8_t* ptr = (uint8_t*)Malloc(32);
    TEST_ASSERT_NOT_NULL(ptr);

    // Fill with test data
    for (int i = 0; i < 32; i++) {
        ptr[i] = (uint8_t)(i + 1);
    }

    // Grow the block
    uint8_t* ptr_grow = (uint8_t*)Realloc(ptr, 64);
    TEST_ASSERT_NOT_NULL(ptr_grow);

    // Verify old data is preserved
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL_UINT8(i + 1, ptr_grow[i]);
    }

    Free(ptr_grow);
    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// ✅ Test 5: Realloc large block to small block (boundary)
void test_Realloc_LargeToSmall(void) {
    uint8_t* ptr = (uint8_t*)Malloc(256);
    TEST_ASSERT_NOT_NULL(ptr);

    memset(ptr, 0xAA, 256);

    uint8_t* ptr_small = (uint8_t*)Realloc(ptr, 16);
    TEST_ASSERT_NOT_NULL(ptr_small);

    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_UINT8(0xAA, ptr_small[i]);
    }

    Free(ptr_small);
    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// ✅ Test 6: Realloc multiple resizes, growing and shrinking
void test_Realloc_MultipleResizes(void) {
    uint8_t* ptr = (uint8_t*)Malloc(32);
    TEST_ASSERT_NOT_NULL(ptr);

    memset(ptr, 0x55, 32);

    // Grow to 64 bytes
    ptr = (uint8_t*)Realloc(ptr, 64);
    TEST_ASSERT_NOT_NULL(ptr);

    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x55, ptr[i]);
    }

    // Shrink to 16 bytes
    ptr = (uint8_t*)Realloc(ptr, 16);
    TEST_ASSERT_NOT_NULL(ptr);

    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x55, ptr[i]);
    }

    Free(ptr);
    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// ✅ Test 7: Realloc NULL with size 0 returns NULL
void test_Realloc_NULL_SizeZero_ReturnsNULL(void) {
    void* ptr = Realloc(NULL, 0);
    TEST_ASSERT_NULL(ptr);

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// ✅ Test 8: Realloc NULL pointer and size > MAX_BLOCK_SIZE returns NULL
void test_Realloc_NULL_TooBig_ReturnsNULL(void) {
    void* ptr = Realloc(NULL, MAX_BLOCK_SIZE + 1);
    TEST_ASSERT_NULL(ptr);

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

// ✅ Test 9: Realloc stress test - allocate, grow, shrink, free
void test_Realloc_Stress(void) {
    const int cycles = 50;
    const size_t start_size = 32;
    const size_t grow_size = 128;
    const size_t shrink_size = 16;

    void* ptrs[cycles];

    // Allocate initial blocks
    for (int i = 0; i < cycles; i++) {
        ptrs[i] = Malloc(start_size);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    // Grow each block
    for (int i = 0; i < cycles; i++) {
        ptrs[i] = Realloc(ptrs[i], grow_size);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    // Shrink each block
    for (int i = 0; i < cycles; i++) {
        ptrs[i] = Realloc(ptrs[i], shrink_size);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    // Free all
    for (int i = 0; i < cycles; i++) {
        Free(ptrs[i]);
    }

    heap_stats_t stats;
    Heap_Stats(&stats);
    TEST_ASSERT_EQUAL_UINT(0, stats.used);
}

/* ============================ */
/*        TEST RUNNER           */
/* ============================ */
int main(void) {
    UNITY_BEGIN();
    
    printf("\n\nStarting Malloc Tests\n\n");
    RUN_TEST(test_MallocZeroSize_ShouldReturnNULL);
    RUN_TEST(test_MallocMoreThanMaxSize);
    RUN_TEST(test_FreeNULLPtr);

    RUN_TEST(test0_Malloc);
    RUN_TEST(test1_Malloc);
    RUN_TEST(test2_Malloc);
    RUN_TEST(test3_Malloc);

    RUN_TEST(test4_Stress_AllocFree_VaryingSizes);
    RUN_TEST(test5_Stress_RepeatedFillAndEmpty);
    RUN_TEST(test6_Stress_AllocateUntilFull);
    RUN_TEST(test7_Stress_Fragmentation);
    RUN_TEST(Test8_MaxingOutMalloc_ThenComparingData); Heap_Init();

    printf("\n\nStarting Calloc Tests\n\n");
    RUN_TEST(test_CallocZeroSize_ShouldReturnNULL);
    RUN_TEST(test_Calloc_SingleAllocation_ZeroInitialized);
    RUN_TEST(test_Calloc_MultipleAllocations_ZeroInitialized);
    RUN_TEST(test_Calloc_Stress);   

    printf("\n\nStarting Realloc Tests\n\n");
    RUN_TEST(test_Realloc_NULL_Pointer_ShouldAllocate);
    RUN_TEST(test_Realloc_SizeZero_ShouldFreeBlock);
    RUN_TEST(test_Realloc_ShrinkBlock_PreservesData);
    RUN_TEST(test_Realloc_GrowBlock_PreservesData);
    RUN_TEST(test_Realloc_LargeToSmall);
    RUN_TEST(test_Realloc_MultipleResizes);
    RUN_TEST(test_Realloc_NULL_SizeZero_ReturnsNULL);
    RUN_TEST(test_Realloc_NULL_TooBig_ReturnsNULL);
    RUN_TEST(test_Realloc_Stress);

    return UNITY_END();
}
