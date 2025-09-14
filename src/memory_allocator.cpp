//
// Created by Angel Dust on 17/07/2025.
//

#include "memory_allocator.h"
#include <cstring>
#include "status.h"

// Static definitions
uint8_t CCMMemoryAllocator::ccm_memory_pool[size] __attribute__((section(".ccmram"))) __attribute__((aligned(32)));
CCMMemoryAllocator::Block CCMMemoryAllocator::blocks[64];
CCMMemoryAllocator::Block *CCMMemoryAllocator::free_blocks = nullptr;
size_t CCMMemoryAllocator::total_allocated = 0;
bool CCMMemoryAllocator::initialized = false;

void CCMMemoryAllocator::init() {
    if (initialized) {
        return;
    }

    // Already zeroed by startup code, so memory_pool is already zero

    // Clear block descriptors (these are also in BSS, so already zero)
    // memset(blocks, 0, sizeof(blocks)); // Not needed - is pre-zeroed

    // Initialize the first free block
    blocks[0].ptr = ccm_memory_pool;
    blocks[0].size = sizeof(ccm_memory_pool);
    blocks[0].free = true;
    blocks[0].next = nullptr;

    free_blocks = &blocks[0];
    total_allocated = 0;
    initialized = true;

    printf_("Allocator initialized: %lu KB in CMM RAM\n", sizeof(ccm_memory_pool) / 1024);
}

void *CCMMemoryAllocator::alloc(size_t size, size_t alignment) {
    if (!initialized) {
        init();
    }

    if (size == 0) {
        return nullptr;
    }

    // Align the size
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);

    // Find a suitable free block
    Block *current = free_blocks;
    Block *prev = nullptr;

    while (current) {
        if (current->free && current->size >= aligned_size) {
            // Found suitable block
            uintptr_t addr = (uintptr_t)current->ptr;
            uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
            void *aligned_ptr = (void *)aligned_addr;
            size_t offset = aligned_addr - addr;
            size_t total_needed = offset + aligned_size;

            if (current->size >= total_needed) {
                // Handle alignment offset
                if (offset > 0) {
                    // Find free block slot for the offset part
                    for (int i = 1; i < 64; i++) {
                        if (blocks[i].ptr == nullptr) {
                            blocks[i].ptr = current->ptr;
                            blocks[i].size = offset;
                            blocks[i].free = true;
                            blocks[i].next = current;

                            if (prev) {
                                prev->next = &blocks[i];
                            } else {
                                free_blocks = &blocks[i];
                            }

                            break;
                        }
                    }

                    current->ptr = aligned_ptr;
                    current->size -= offset;
                }

                // Mark as allocated
                current->free = false;

                // Split if there's remaining space
                if (current->size > aligned_size) {
                    for (int i = 1; i < 64; i++) {
                        if (blocks[i].ptr == nullptr) {
                            blocks[i].ptr = (uint8_t *)current->ptr + aligned_size;
                            blocks[i].size = current->size - aligned_size;
                            blocks[i].free = true;
                            blocks[i].next = current->next;
                            current->next = &blocks[i];
                            break;
                        }
                    }
                    current->size = aligned_size;
                }

                total_allocated += aligned_size;

                // Memory is already zeroed (property)
                return current->ptr;
            }
        }
        prev = current;
        current = current->next;
    }

    status::pop_alert(status::ST_ERROR, "CCM memory allocation failed\n");
    return nullptr;
}

void CCMMemoryAllocator::free(void *ptr) {
    if (!ptr || !initialized) {
        return;
    }

    // Find the block
    for (int i = 0; i < 64; i++) {
        if (blocks[i].ptr == ptr && !blocks[i].free) {
            blocks[i].free = true;
            total_allocated -= blocks[i].size;

            // Simple coalescing with next block
            if (blocks[i].next && blocks[i].next->free) {
                Block *next = blocks[i].next;
                if ((uint8_t *)blocks[i].ptr + blocks[i].size == (uint8_t *)next->ptr) {
                    blocks[i].size += next->size;
                    blocks[i].next = next->next;
                    memset(next, 0, sizeof(Block));
                }
            }
            return;
        }
    }

    printf("CMM free: invalid pointer\n");
}

void CCMMemoryAllocator::print_usage() {
    printf("Allocator Usage:\n");
    printf("  Total:     %lu bytes\n", sizeof(ccm_memory_pool));
    printf("  Allocated: %lu bytes (%.1f%%)\n", total_allocated, (float)total_allocated / sizeof(ccm_memory_pool) * 100.0f);
    printf("  Free:      %lu bytes (%.1f%%)\n", sizeof(ccm_memory_pool) - total_allocated,
           (float)(sizeof(ccm_memory_pool) - total_allocated) / sizeof(ccm_memory_pool) * 100.0f);
}
