//
// Created by Angel Dust on 17/07/2025.
//

#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include <stdio.h>
#include <stdint.h>

class CCMMemoryAllocator {
  public:
    struct Block {
        void *ptr;
        size_t size;
        bool free;
        Block *next;
    };

  private:
    static Block blocks[64];
    static Block *free_blocks;
    static size_t total_allocated;
    static bool initialized;
    static constexpr int size = 16384;

  public:
    static uint8_t ccm_memory_pool[size] __attribute__((section(".ccmram"))) __attribute__((aligned(32)));
    static void init();
    static void *alloc(size_t size, size_t alignment = 4);
    static void free(void *ptr);
    static void print_usage();
};

#endif
