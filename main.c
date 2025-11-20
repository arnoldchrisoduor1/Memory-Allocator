#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

// Alignment for memory addresses (8 bytes for 64-bit systems)
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

// Block metadata structure
typedef struct block {
    size_t size;          // Size of usable memory (excluding header)
    int free;             // 1 = free, 0 = allocated
    struct block *next;   // Next block in list
} block_t;

#define BLOCK_SIZE sizeof(block_t)

// Global head of free list
static block_t *head = NULL;

// ========== Helper Functions ==========

// Get block header from user pointer
block_t *get_block_ptr(void *ptr) {
    return (block_t*)ptr - 1;
}

// Request memory from OS using sbrk()
block_t *request_space(block_t *last, size_t size) {
    block_t *block;
    block = sbrk(0);  // Get current program break
    
    void *request = sbrk(BLOCK_SIZE + size);
    if (request == (void*) -1) {
        return NULL;  // sbrk failed
    }
    
    if (last) {
        last->next = block;
    }
    
    block->size = size;
    block->free = 0;
    block->next = NULL;
    return block;
}

// ========== Allocation Strategies ==========

// First-fit: Find first block large enough
block_t *find_free_block_first_fit(block_t **last, size_t size) {
    block_t *current = head;
    
    while (current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    
    return current;
}

// Best-fit: Find smallest block that fits
block_t *find_free_block_best_fit(block_t **last, size_t size) {
    block_t *current = head;
    block_t *best = NULL;
    size_t best_size = SIZE_MAX;
    
    *last = NULL;
    
    while (current) {
        if (current->free && current->size >= size) {
            if (current->size < best_size) {
                best = current;
                best_size = current->size;
            }
        }
        if (!best || current == best) {
            *last = current;
        }
        current = current->next;
    }
    
    return best;
}

// Split a block if it's too large
void split_block(block_t *block, size_t size) {
    // Only split if remainder is useful (at least ALIGNMENT bytes + header)
    if (block->size >= size + BLOCK_SIZE + ALIGNMENT) {
        // Create new block in the remaining space
        block_t *new_block = (block_t*)((char*)(block + 1) + size);
        new_block->size = block->size - size - BLOCK_SIZE;
        new_block->free = 1;
        new_block->next = block->next;
        
        block->size = size;
        block->next = new_block;
    }
}

// Merge adjacent free blocks
void coalesce() {
    block_t *current = head;
    
    while (current && current->next) {
        if (current->free && current->next->free) {
            // Merge current with next
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;
            // Don't advance - check if next one can also merge
        } else {
            current = current->next;
        }
    }
}

// ========== Public API ==========

void *my_malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }
    
    // Align size for performance and correctness
    size = ALIGN(size);
    
    block_t *block;
    
    if (!head) {
        // First allocation ever
        block = request_space(NULL, size);
        if (!block) {
            return NULL;
        }
        head = block;
    } else {
        block_t *last = head;
        
        // Try to find a free block (using first-fit strategy)
        block = find_free_block_first_fit(&last, size);
        
        if (!block) {
            // No free block found - request more memory
            block = request_space(last, size);
            if (!block) {
                return NULL;
            }
        } else {
            // Found a free block - split if too large
            split_block(block, size);
            block->free = 0;
        }
    }
    
    // Return pointer to usable memory (after header)
    return (block + 1);
}

void my_free(void *ptr) {
    if (!ptr) {
        return;
    }
    
    // Get block header
    block_t *block = get_block_ptr(ptr);
    block->free = 1;
    
    // Merge adjacent free blocks
    coalesce();
}

void *my_realloc(void *ptr, size_t size) {
    if (!ptr) {
        return my_malloc(size);
    }
    
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }
    
    block_t *block = get_block_ptr(ptr);
    
    if (block->size >= size) {
        // Current block is large enough
        split_block(block, size);
        return ptr;
    }
    
    // Need to allocate new block
    void *new_ptr = my_malloc(size);
    if (!new_ptr) {
        return NULL;
    }
    
    // Copy old data to new location
    memcpy(new_ptr, ptr, block->size);
    my_free(ptr);
    
    return new_ptr;
}

// ========== Debug/Visualization Functions ==========

void print_memory_map() {
    block_t *current = head;
    int block_num = 0;
    
    printf("\n=== Memory Map ===\n");
    while (current) {
        printf("Block %d: [%s] size=%zu bytes, next=%p\n",
               block_num++,
               current->free ? "FREE" : "USED",
               current->size,
               (void*)current->next);
        current = current->next;
    }
    printf("==================\n\n");
}

// ========== Test Program ==========

int main() {
    printf("Custom Memory Allocator Demo\n\n");
    
    // Test 1: Basic allocation
    printf("Test 1: Allocating 3 blocks\n");
    int *a = my_malloc(sizeof(int) * 10);
    int *b = my_malloc(sizeof(int) * 20);
    int *c = my_malloc(sizeof(int) * 5);
    
    if (a) a[0] = 100;
    if (b) b[0] = 200;
    if (c) c[0] = 300;
    
    print_memory_map();
    
    // Test 2: Free middle block
    printf("Test 2: Freeing middle block\n");
    my_free(b);
    print_memory_map();
    
    // Test 3: Allocate into freed space
    printf("Test 3: Allocating into freed space\n");
    int *d = my_malloc(sizeof(int) * 15);
    if (d) d[0] = 400;
    print_memory_map();
    
    // Test 4: Free adjacent blocks (should coalesce)
    printf("Test 4: Freeing adjacent blocks (coalescing)\n");
    my_free(a);
    my_free(c);
    print_memory_map();
    
    // Test 5: Realloc
    printf("Test 5: Reallocating block\n");
    d = my_realloc(d, sizeof(int) * 30);
    if (d) printf("Realloc successful, d[0] = %d\n", d[0]);
    print_memory_map();
    
    // Cleanup
    my_free(d);
    
    printf("All tests completed!\n");
    return 0;
}