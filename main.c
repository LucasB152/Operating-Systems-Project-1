/*
* This file allows to test your buddy allocator implementation. It is divided into 2 parts based on the variables {MIN|MAX}_ALLOC_SIZE_BITS.
* 1) The first part (test_minimal) can be used to quickly check that your implementation is working correctly for small sizes.
*    It also allows you to debug your implementation more easily by providing a simple test case with a small memory size.
* 2) The second part (test_advanced) tests more complex scenarios, such as allocating and freeing various sizes of memory blocks.
*
* By default, the first test is run. If you want to run the second test, you must set appropriate values for MIN_ALLOC_SIZE_BITS and
* MAX_ALLOC_SIZE_BITS in binary_size.h, and recompile the code. The appropriate values can be found in the main at the bottom of this file.
*
* For the second test, we recommend having at least 4GB of (free) memory to run, so make sure to have enough memory available before running the tests.
*   
* You can modify the test cases or add new ones to further test your implementation. The test cases provided here are not exhaustive, but 
* they cover a range of scenarios that should help you identify common issues in buddy allocator implementations.
*
* Compile and run it with: gcc binary_buddy.c main.c -Wall -Wextra -pedantic -o main -lm && ./main
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "binary_buddy.h"

#define N 10 // for advanced test
#define MAX(a,b) (((a)>(b))?(a):(b))
#define ALLOC(type, count) \
    (type*) alloc_generic((count), sizeof(type))

static const size_t MB = (1024 * 1024);

int test_minimal(const void* base_addr) {

    printf("[Test minimal start]\n");

    const int hole = 4;
    char* ptrs[] = {NULL, NULL, NULL, NULL, NULL};

    // Allocate 4 bytes
    ptrs[0] = balloc(4);
    assert(ptrs[0] != NULL);
    printf(" - Allocated char at address %p, total used space: %lu bytes\n", (void*)ptrs[0], get_used_space());
    assert(get_used_space() == 4);
    assert(ptrs[0] == (char*)base_addr);
    // [0-3] allocated, [4-7] free, [8-15] free

    // Allocate 8 bytes
    ptrs[1] = balloc(8);
    assert(ptrs[1] != NULL);
    printf(" - Allocated char at address %p, total used space: %lu bytes\n", (void*)ptrs[1], get_used_space());
    assert(get_used_space() == 12);
    assert(ptrs[1] == (char*)((size_t)base_addr + 4 + hole)); // Check that the allocated address is correct
    // [0-3] allocated, [4-7] free, [8-15] allocated

    // Allocate 2 bytes
    ptrs[2] = balloc(2);
    assert(ptrs[2] != NULL);
    printf(" - Allocated char at address %p, total used space: %lu bytes\n", (void*)ptrs[2], get_used_space());
    assert(get_used_space() == 14);
    assert(ptrs[2] == (char*)((size_t)base_addr + 4));
    // [0-3] allocated, [4-5] allocated, [6-7] free, [8-15] allocated

    // Free pointer 1 to check that the allocator correctly merges the free block
    bfree(ptrs[1]);
    assert(get_used_space() == 6);
    // [0-3] allocated, [4-5] allocated, [6-7] free, [8-15] free

    // Allocate 4 bytes
    ptrs[1] = balloc(4);
    assert(ptrs[1] != NULL);
    printf(" - Allocated char at address %p, total used space: %lu bytes\n", (void*)ptrs[1], get_used_space());
    assert(get_used_space() == 10);
    assert(ptrs[1] == (char*)((size_t)base_addr + 4 + hole)); // Check that the allocated address is correct
    // [0-3] allocated, [4-5] allocated, [6-7] free, [8-11] allocated, [12-15] free

    // Allocate 4 bytes
    ptrs[3] = balloc(4);
    assert(ptrs[3] != NULL);
    printf(" - Allocated char at address %p, total used space: %lu bytes\n", (void*)ptrs[3], get_used_space());
    assert(get_used_space() == 14);
    assert(ptrs[3] == (char*)((size_t)base_addr + 4 + hole + 4)); // Check that the allocated address is correct
    // [0-3] allocated, [4-5] allocated, [6-7] free, [8-11] allocated, [12-15] allocated

    // Allocate 4 bytes, should fail because there is not enough space
    ptrs[4] = balloc(4);
    assert(ptrs[4] == NULL);
    assert(get_used_space() == 14);
    // [0-3] allocated, [4-5] allocated, [6-7] free, [8-11] allocated, [12-15] allocated

    // Allocate 2 bytes, should succeed because there is enough space in the free block [6-7]
    ptrs[4] = balloc(2);
    assert(ptrs[4] != NULL);
    printf(" - Allocated char at address %p, total used space: %lu bytes\n", (void*)ptrs[4], get_used_space());
    assert(get_used_space() == MAX_ALLOC_SIZE);
    assert(ptrs[4] == (char*)((size_t)base_addr + 6));
    // [0-3] allocated, [4-5] allocated, [6-7] allocated, [8-11] allocated, [12-15] allocated -> All space is allocated

    // Allocate 2 bytes, should fail because there is no more space
    void* tmp = balloc(2);
    assert(tmp == NULL);
    assert(get_used_space() == MAX_ALLOC_SIZE);
    // [0-3] allocated, [4-5] allocated, [6-7] allocated, [8-11] allocated, [12-15] allocated -> All space is allocated

    // Free all allocated blocks
    for (int i = 0; i < 5; i++) {
        bfree(ptrs[i]);
    }
    assert(get_used_space() == 0);
    printf("[Test minimal passed]\n");

    return 1;
}

static void* alloc_generic(size_t count, size_t elem_size) {
    size_t bytes = count * elem_size;
    printf("   - Allocating %zu bytes\n", bytes);
    void* p = balloc(bytes);
    assert(p != NULL);
    memset(p, 1, bytes);
    // Verify that the allocated memory is correctly initialized -> Only for debugging, can be removed for performance
    for (size_t i = 0; i < bytes; i++) {
        assert(*((unsigned char*)p + i) == 0x01);
    }
    return p;
}

int test_advanced(int mb_size){
    
    printf("[Test advanced start]\n");

    // Allocate and free various sizes, checking for correct behavior 
    void* ptrs[12];
    ptrs[0]  = ALLOC(char,   mb_size);
    ptrs[1]  = ALLOC(int,    10  * mb_size);
    ptrs[2]  = ALLOC(double, 5  * mb_size);
    ptrs[3]  = ALLOC(int,    20  * mb_size);
    ptrs[4]  = ALLOC(float,  2  * mb_size);
    ptrs[5]  = ALLOC(double, 10 * mb_size);
    ptrs[6]  = ALLOC(char,   5  * mb_size);
    ptrs[7]  = ALLOC(char,   7  * mb_size);
    ptrs[8]  = ALLOC(int,    15 * mb_size);
    ptrs[9]  = ALLOC(float,  25 * mb_size);
    ptrs[10] = ALLOC(char,   10 * mb_size);
    ptrs[11] = ALLOC(char,   2  * mb_size);

    assert(get_used_space() != 0);

    for (int i = 0; i < 12; i++) {
        bfree(ptrs[i]);
    }

    assert(get_used_space() == 0);
    ptrs[0]  = ALLOC(char, mb_size);
    ptrs[1]  = ALLOC(int, 10 * mb_size);
    ptrs[2]  = ALLOC(double, 5 * mb_size);
    bfree(ptrs[0]);
    ptrs[3]  = ALLOC(int, 20 * mb_size);
    ptrs[4]  = ALLOC(float, 2 * mb_size);
    bfree(ptrs[4]);
    ptrs[5]  = ALLOC(double, 10 * mb_size);
    ptrs[6]  = ALLOC(char, 5 * mb_size);
    bfree(ptrs[5]);
    bfree(ptrs[6]);
    bfree(ptrs[1]);
    bfree(ptrs[2]);
    ptrs[7]  = ALLOC(char, 7 * mb_size);
    bfree(ptrs[3]);
    bfree(ptrs[7]);
    ptrs[8]  = ALLOC(int, 15 * mb_size);
    ptrs[9]  = ALLOC(float, 25 * mb_size);
    bfree(ptrs[8]);
    bfree(ptrs[9]);
    ptrs[10] = ALLOC(char, 100 * mb_size);
    bfree(ptrs[10]);
    assert(get_used_space() == 0);

    // Test random alloc/free pattern
    srand(1);
    size_t sz = 0;
    char* ptrs_chars[N];
    memset(ptrs_chars, 0, sizeof(ptrs_chars));
    for (int iter = 0; iter < N; iter++) {
        int i = rand() % N;
        if (ptrs_chars[i]) {
            bfree(ptrs_chars[i]);
            ptrs_chars[i] = NULL;
        } else {
            if (i % 2 == 0) {
                sz =  MAX(1, rand() % MIN_ALLOC_SIZE);
            } else {
                sz = MIN_ALLOC_SIZE + rand() % (MAX_ALLOC_SIZE/N - MIN_ALLOC_SIZE);
            }
            ptrs_chars[i] = ALLOC(char, sz);
        }
    }
    assert(get_used_space() != 0);

    // Free any remaining allocated blocks
    for (int i = 0; i < N; i++) {
        if (ptrs_chars[i]) {
            bfree(ptrs_chars[i]);
            ptrs_chars[i] = NULL;
        }
    }
    assert(get_used_space() == 0);
    
    printf("[Test advanced passed]\n");
    return 1;
}

int main(){

    printf("- MIN_ALLOC_SIZE: %lu bytes (n_bits: %u)\n", MIN_ALLOC_SIZE, MIN_ALLOC_SIZE_BITS);
    printf("- MAX_ALLOC_SIZE: %lu bytes (n_bits: %u)\n", MAX_ALLOC_SIZE, MAX_ALLOC_SIZE_BITS);
    printf("You can change the MAX and MIN alloc size values to access different tests. Check out in the main function at the end of main.c.\n");

    void *base = init_buddy(MAX_ALLOC_SIZE);
    if (base == NULL) {
        fprintf(stderr,"Failed to initialize buddy allocator\n");
        return 1;
    }
    printf("Memory base allocated at: %p (size: %lu bytes)\n", base, MAX_ALLOC_SIZE);

    if (MAX_ALLOC_SIZE_BITS == 4 && MIN_ALLOC_SIZE_BITS == 1) {
        if (test_minimal(base) != 1) {
            fprintf(stderr, "Test minimal failed!\n");
            return 1;
        }
    }else if (MAX_ALLOC_SIZE_BITS == 32 && MIN_ALLOC_SIZE_BITS == 20) {
        if (test_advanced(MB) != 1) {
            fprintf(stderr, "Test advanced failed!\n");
            return 1;
        }
    }else{
        fprintf(stderr, "No test defined for this configuration (MIN_ALLOC_SIZE_BITS: %u, MAX_ALLOC_SIZE_BITS: %u). Make your own tests!\n", MIN_ALLOC_SIZE_BITS, MAX_ALLOC_SIZE_BITS);
        return 1;
    }

    if (free_buddy() != 0) {
        fprintf(stderr, "Failed to free buddy allocator\n");
        return 1;
    }

    return 0;
}
