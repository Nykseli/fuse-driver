// https://www.cs.princeton.edu/courses/archive/spr09/cos217/lectures/19DynamicMemory2.pdf

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __USE_MISC
#define __USE_MISC
#endif
#include <unistd.h>

// malloc uses 0x21000
#define ALLOC_CHUNK 0x21000
#define CHUNK_SIZE(_size) (_size / ALLOC_CHUNK + 1) * ALLOC_CHUNK

// TODO: test to see if mem has been leaked after runtime is over

typedef struct total_mem {
    // pointer to sbrk memory
    uint8_t* mem_start;
    uint8_t* mem_tail;
    size_t size;
} total_mem;

/**
 * Item in brk memory
 */
typedef struct fsmem {
    size_t size; // size of data, doesn't include size of fsmem
    /* next is the next reserved or freed mem */
    uint32_t flags;
    struct fsmem* next;
    /* struct fsmem* prev; */
    // data is only thing that's exposed to enduser
    uint8_t* data;
} fsmem;

fsmem* free_head = NULL;
fsmem* free_tail = NULL;
fsmem resv_mem;
fsmem* resv_tail = NULL;
total_mem sbrk_mem = {
    .mem_start = NULL,
    .mem_tail = NULL,
    .size = 0,
};

static fsmem* find_next_freed(size_t size) {
    // TODO: check mem allocated next to the free mem is also free and
    //       combine them to a larger chunk
    fsmem* found = free_head;
    fsmem* prev = NULL;
    while (found != NULL) {
        if (found->size <= size) {
            // "remove" the found from frees
            if (prev != NULL)
                prev->next = found->next;
            else
                free_head = found->next;

            found->next = NULL;
            found->flags = 0;
            return found;
        }

        prev = found;
        found = found->next;
    }

    return NULL;
}

static fsmem* alloc_new(size_t size) {
    // If we need new fsmem, first make we have space for it
    size_t new_size = size + sizeof(fsmem);
    size_t mem_free = sbrk_mem.size - (sbrk_mem.mem_tail - sbrk_mem.mem_start);
    if (new_size > mem_free) {
        // Allocate at least 1 new chunk
        size_t all_size = CHUNK_SIZE(size);
        // TODO: error handling
        sbrk(all_size); // We don't need the addres to the new area
        sbrk_mem.size += all_size;
    }

    fsmem* new_item = (fsmem*)sbrk_mem.mem_tail;
    new_item->size = size;
    new_item->next = NULL;
    new_item->data = sbrk_mem.mem_tail + sizeof(fsmem);
    new_item->flags = 0;
    sbrk_mem.mem_tail += new_size;
    return new_item;
}

void* fsalloc(size_t size) {
    // Could we do this else where so we didn't need to check this on every call
    if (sbrk_mem.size == 0) {
        size_t asize = CHUNK_SIZE(size);
        sbrk_mem.mem_start = sbrk(asize);
        sbrk_mem.mem_tail = sbrk_mem.mem_start;
        sbrk_mem.size = asize;
    }

    // If there's freed mem, prioritise using it
    fsmem* free = find_next_freed(size);
    if (free != NULL)
        return (void*)free->data;

    fsmem* new_item = alloc_new(size);
    return (void*)new_item->data;
}

void* fscalloc(size_t nmemb, size_t size) {
    size = nmemb * size;
    return NULL;
}

// TODO: reduce brk size at somepoint?
void fsfree(void* mem) {
    fsmem* item = (fsmem*)((uint8_t*)mem - sizeof(fsmem));
    if (item->flags == 1) {
        printf("fsfree double free\n");
        exit(1);
    }

    item->flags = 1;

    item->next = NULL; // this is going to be last item
    if (free_head == NULL) {
        free_head = item;
        free_tail = item;
    } else {
        free_tail->next = item;
        free_tail = item;
    }
}

void* fsrealloc(void* mem, size_t size) {
    fsmem* item = (fsmem*)((uint8_t*)mem - sizeof(fsmem));

    // If the item already has space for it, just return it
    if (item->size >= size) {
        return (void*)item->data;
    }

    // otherwise free the current mem and find a new one
    fsfree(mem);
    fsmem* new_item = find_next_freed(size);
    if (new_item == NULL)
        new_item = alloc_new(size);

    memcpy(new_item->data, item->data, size);
    return (void*)new_item->data;
}
