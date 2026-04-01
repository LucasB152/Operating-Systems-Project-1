#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "binary_buddy.h"

#include <stdlib.h>

/*
* Initalize the structures needed for the buddy allocator.
* Returns 0 on success, -1 on failure.
*/
static int init_structures(const void* memory_base, size_t size);

/*
* Free the buddy allocator's internal data structures.
*/
static void free_structures();

/*
 * Round the size to next closed power of two
*/
static size_t round_up_pow2(size_t x);

/*
 * Insert a free node in the free list of the given level, keeping the list sorted by node index.
*/
static void insert_free(int32_t node_idx, int level);

/*
 * Remove a free node from the free list of the given level.
*/
static void remove_free(int32_t node_idx, int level);

// ------------------- START PROTECTED CODE -------------------

Allocator a;

void* init_buddy(size_t size){
    void* memory_base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_base == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    
    if (init_structures(memory_base, size) != 0) { 
        munmap(memory_base, size);
        return NULL;
    }
    
    return memory_base;
}

int free_buddy() {
    free_structures();
    return munmap((void*)a.base, a.total_size);
}

size_t get_used_space(){
    return a.used_space;
}

// ------------------- END PROTECTED CODE -------------------

static size_t round_up_pow2(size_t x)
{
    if (x <= 1) return 1;
    if ((x & (x - 1)) == 0) return x; // Already a power of 2
    return 1UL << (64 - __builtin_clzl(x)); // Round up to the next power of 2
}

static void insert_free(int32_t node_idx, int level)
{
    int32_t current = a.level_heads[level];
    int32_t prev_node = -1;

    while (current != -1 && current < node_idx) {
        prev_node = current;
        current = a.nodes[current].next;
    }

    a.nodes[node_idx].next = current;
    a.nodes[node_idx].prev = prev_node;
    a.nodes[node_idx].status = NODE_FREE;

    if (prev_node != -1) {
        a.nodes[prev_node].next = node_idx;
    } else {
        a.level_heads[level] = node_idx; // adjust the head of level
    }

    if (current != -1) {
        a.nodes[current].prev = node_idx;
    }
}

static void remove_free(int32_t node_idx, int level)
{
    int32_t prev_node =a.nodes[node_idx].prev;
    int32_t next_node = a.nodes[node_idx].next;

    if (prev_node != -1) {
        a.nodes[prev_node].next = next_node;
    } else {
        a.level_heads[level] = next_node;
    }

    if (next_node != -1) {
        a.nodes[next_node].prev = prev_node;
    }

    a.nodes[node_idx].prev = -1;
    a.nodes[node_idx].next = -1;
}

void* balloc(size_t size) {

    if (size == 0) {
        size = 1;
    }

    size_t alloc_size = round_up_pow2(size);
    if (alloc_size < a.min_size) {
        alloc_size = a.min_size;
    }
    if (alloc_size > a.total_size) {
        return NULL;
    }

    int l_target = __builtin_ctzll(a.total_size / alloc_size);

    int l_found = -1;
    for (int l = l_target; l >= 0; --l) {
        if (a.level_heads[l] != -1) {
            l_found = l;
            break;
        }
    }
    if (l_found == -1) {
        return NULL;
    }

    int32_t current = a.level_heads[l_found];
    remove_free(current, l_found);

    for (int level = l_found; level < l_target; ++level) {
        a.nodes[current].status = NODE_SPLIT;

        int32_t left = 2 * current + 1;
        int32_t right = 2 * current + 2;

        a.nodes[right].status = NODE_FREE;
        insert_free(right, level + 1);

        current = left;
    }

    a.nodes[current].status = NODE_USED;

    size_t level_base = (1ULL << l_target) - 1ULL; // Node array index of the first block in the level
    size_t k = (size_t)current - level_base; // Index of the block in the level
    size_t min_idx = k * (alloc_size / a.min_size); // Index of the minimal block after which the block start

    a.allocated_levels[min_idx] = (uint8_t)l_target;
    a.used_space += alloc_size;

    return (void *)((char *)a.base + min_idx * a.min_size);
}

void bfree(void* ptr) {

    if (ptr == NULL) {
        return;
    }

    size_t min_index = ((char*)ptr - (char*)a.base) / a.min_size;

    // Recover the level of the node and his index in the level
    int level = a.allocated_levels[min_index];
    size_t block_size = a.total_size >> level;

    size_t k =  ((char*)ptr - (char*)a.base) / block_size; // Index of the node in the level
    int32_t node_idx = (1ULL << level) - 1ULL + k; // Index of the node in the node array

    a.nodes[node_idx].status = NODE_FREE;
    insert_free(node_idx, level);
    a.used_space -= block_size;

    // Try merging with his buddy, if free
    while (level > 0) {
        int32_t buddy = (node_idx % 2 == 1) ? (node_idx + 1) : (node_idx - 1);

        if (a.nodes[buddy].status != NODE_FREE) {
            break;
        }

        remove_free(node_idx, level);
        remove_free(buddy, level);

        node_idx = (node_idx - 1) / 2;
        level -= 1;

        a.nodes[node_idx].status = NODE_FREE;
        insert_free(node_idx, level);
    }
}

static int init_structures(const void* memory_base, size_t size){

    a.base = memory_base;
    a.total_size = size;
    a.used_space = 0;

    // Compute min and maximum bytes size
    a.min_size = MIN_ALLOC_SIZE;
    size_t N = size / a.min_size; // Number of blocks of minimum size
    a.num_levels = MAX_ALLOC_SIZE_BITS - MIN_ALLOC_SIZE_BITS + 1;

    a.nodes = malloc((2 * N -1) * sizeof(Node));
    a.level_heads = malloc(a.num_levels * sizeof(int32_t));
    a.allocated_levels = malloc(N * sizeof(uint8_t));

    if (a.nodes == NULL || a.level_heads == NULL || a.allocated_levels == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for allocator structure\n"); //TODO: To be removed before submission
        return -1;
    }

    for (int32_t i = 0; i < (int32_t)(2 * N - 1); ++i)  {
        a.nodes[i].status = NODE_FREE; //TODO: think about removing tis from here
        a.nodes[i].prev = -1;
        a.nodes[i].next = -1;
    }

    for (int32_t level = 0; level < a.num_levels; ++level) {
        a.level_heads[level] = -1;
    }

    a.nodes[0].status = NODE_FREE;
    a.level_heads[0] = 0;

    return 0;
}

static void free_structures() {

    free(a.nodes);
    free(a.level_heads);
    free(a.allocated_levels);
}
