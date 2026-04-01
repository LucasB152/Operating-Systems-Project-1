#ifndef BINARY_BUDDY_H
#define BINARY_BUDDY_H
#define NODE_FREE  0
#define NODE_SPLIT 1
#define NODE_USED  2


#include <stddef.h>
#include <stdint.h>

#include "buddy_size.h"

// You can add code/struct here if needed
typedef struct {
    uint8_t status;
    int32_t prev;
    int32_t next;
} Node;

// ------------------- START PROTECTED CODE -------------------

/* 
* Initialize the buddy allocator with a memory region of given size.
* Returns a pointer to the base of the memory region on success, NULL on failure.
*/
void* init_buddy(size_t size);

/*
* Release the memory region used by the buddy allocator.
* Returns 0 on success, -1 on failure.
*/
int free_buddy();

/* 
* Allocate a memory chunk of given size.
* Returns a pointer to the allocated chunk, NULL on failure.
*/
void* balloc(size_t size);

/* 
* Deallocate a previously allocated memory chunk.
*/
void bfree(void* ptr);

/*
* Get the total used space in bytes in the allocator.
*/
size_t get_used_space();

typedef struct {
    // The 3 fields below are protected
    const void* base; // Base address of the memory region
    size_t used_space; // Total used space in the allocator
    size_t total_size; // Total size of the memory region

// ------------------- END PROTECTED CODE -------------------

    // Add some fields here
    int num_levels;
    size_t min_size;
    Node *nodes;
    int32_t *level_heads;
    uint8_t *allocated_levels;

} Allocator; // Cannot modify the name of this struct, but you can add fields to it

// You can add code/struct here if needed

#endif // BINARY_BUDDY_H
