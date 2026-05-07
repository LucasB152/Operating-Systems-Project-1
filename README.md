# Buddy System Allocator

> Project 1 — INFO0940-1: Operating Systems  
> University of Liège | Prof. L. Mathy, B. Knott, G. Gain

## Overview

A user-space implementation of a **binary buddy allocator** in C. The buddy system is a classic memory allocation algorithm used notably in the Linux kernel. It manages memory as a collection of power-of-two-sized chunks, splitting and merging them on demand to minimize fragmentation.

Rather than operating in kernel space, this allocator manages a fixed memory region obtained via `mmap`, simulating kernel allocator behaviour from user space.

## How It Works

- Memory is managed as a binary tree of power-of-two-sized chunks.
- When an allocation is requested, the smallest fitting free chunk is split recursively until the right size is reached. The two halves of a split are called **buddies**.
- When a chunk is freed, its buddy is checked. If the buddy is also free, they are **merged** back into a larger chunk — this process propagates up the tree.
- Allocation policy: always pick the free chunk with the **lowest base address** among all chunks of adequate size.

```
          256 KB
         /      \
     128 KB    128 KB
     /    \
  64 KB  64 KB
  /    \
32 KB  32 KB
```

## Project Structure

```
.
├── binary_buddy.c   # Allocator implementation
├── binary_buddy.h   # Allocator interface & data structures
├── buddy_size.h     # MIN_ALLOC_SIZE_BITS / MAX_ALLOC_SIZE_BITS constants
└── main.c           # Test file
```

## API

```c
// Initialize the allocator over a given memory region
int init_structures(const void *memory_base, size_t size);

// Free the allocator's internal data structures
void free_structures();

// Allocate at least `size` bytes using the buddy system
void *balloc(size_t size);

// Free a previously allocated chunk and merge buddies
void bfree(void *ptr);
```

## Building & Testing

```bash
gcc binary_buddy.c main.c -Wall -Wextra -pedantic -o main -lm && ./main
```

## Performance

The buddy system achieves **O(log n)** allocation and deallocation time, where n is the number of levels in the tree. The goal is to be faster than `malloc`.

## Notes

- `balloc` and `bfree` do **not** request memory from the kernel — they are logical managers of the pre-allocated `mmap` region.
- `MIN_ALLOC_SIZE_BITS` and `MAX_ALLOC_SIZE_BITS` in `buddy_size.h` control allocation granularity. The implementation handles any reasonable values of these constants.
