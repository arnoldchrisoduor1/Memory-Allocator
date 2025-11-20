#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT)) & ~(ALIGNMENT-1))

typedef struct block {
    size_t size;
    int free;
    struct block *next;
} block_t;

#define BLOCK_SIZE sizeof(block_t)

static block_t *head = NULL;

block_t *get_block_ptr(void *ptr) {
    return (block_t*)ptr - 1;
}

block_t *request_space(block_t *last, size_t size) {
    block_t *block;
    block = sbrk(0);

    void *request = srbk(BLOCK_SIZE + size);
    if(request == (void*) -1) {
        return NULL;
    }

    if (last) {
        last->next = block;
    }

    block->size = size;
    block->free = 0;
    block->next = NULL;
    return block;
}

block_t *find_free_block_first_fit(block_t **last, size_t size) {
    block_t *current = head;

    while(current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

block_t *find_free_block_best_fit(block_t **last, size_t size) {
    block_t *current = head;
    block_t *best = NULL;
    size_t best_size = SIZE_MAX;

    *last = NULL;

    while (current) {
        if (current->free && current->size >= size) {
            if(current->size < best_size) {
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

void split_block(block_t *block, size_t size) {
    if (block->size >=size + BLOCK_SIZE + ALIGNMENT) {
        block_t *new_block = (block_t*)((char*)(block + 1) + size);
        new_block->size = block->size - size - BLOCK_SIZE;
        new_block->next = block->next;

        block->size = size;
        block->next = new_block;
    }
}