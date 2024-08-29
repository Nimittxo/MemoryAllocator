#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#define TINY_MAX_SIZE 256
#define SMALL_MAX_SIZE 4096
#define LARGE_THRESHOLD 4096
#define NUM_SIZE_CLASSES 32
#define PAGE_SIZE 4096

typedef struct block_header {
    size_t size;
    int is_free;
    struct block_header* next;
    struct block_header* prev;
} block_header_t;

typedef struct size_class {
    size_t size;
    block_header_t* free_list;
} size_class_t;

typedef struct heap {
    size_class_t size_classes[NUM_SIZE_CLASSES];
    block_header_t* large_blocks;
    pthread_mutex_t lock;
} heap_t;

static heap_t global_heap;

void init_heap() {
    memset(&global_heap, 0, sizeof(heap_t));
    pthread_mutex_init(&global_heap.lock, NULL);

    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        global_heap.size_classes[i].size = 8 << i;
    }
}

static inline size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static void* mmap_alloc(size_t size) {
    size = align_up(size, PAGE_SIZE);
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return NULL;
    }
    return ptr;
}

static void mmap_free(void* ptr, size_t size) {
    size = align_up(size, PAGE_SIZE);
    munmap(ptr, size);
}

static block_header_t* split_block(block_header_t* block, size_t size) {
    size_t remaining = block->size - size - sizeof(block_header_t);
    if (remaining < sizeof(block_header_t)) {
        return block;
    }

    block_header_t* new_block = (block_header_t*)((char*)block + sizeof(block_header_t) + size);
    new_block->size = remaining - sizeof(block_header_t);
    new_block->is_free = 1;
    new_block->next = block->next;
    new_block->prev = block;

    if (block->next) {
        block->next->prev = new_block;
    }
    block->next = new_block;
    block->size = size;

    return block;
}

static void coalesce(block_header_t* block) {
    if (block->next && block->next->is_free) {
        block->size += block->next->size + sizeof(block_header_t);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    if (block->prev && block->prev->is_free) {
        block->prev->size += block->size + sizeof(block_header_t);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
}

void* malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    pthread_mutex_lock(&global_heap.lock);

    size = align_up(size, 8);
    block_header_t* block = NULL;

    if (size <= TINY_MAX_SIZE) {
        int class_index = (size - 1) / 8;
        size_class_t* size_class = &global_heap.size_classes[class_index];

        if (!size_class->free_list) {
            void* new_region = mmap_alloc(PAGE_SIZE);
            if (!new_region) {
                pthread_mutex_unlock(&global_heap.lock);
                return NULL;
            }

            block_header_t* new_block = (block_header_t*)new_region;
            new_block->size = PAGE_SIZE - sizeof(block_header_t);
            new_block->is_free = 1;
            new_block->next = NULL;
            new_block->prev = NULL;

            size_class->free_list = new_block;
        }

        block = size_class->free_list;
        size_class->free_list = block->next;
        block = split_block(block, size_class->size);
    } else if (size <= SMALL_MAX_SIZE) {
        int class_index = (size - 1) / 256 + NUM_SIZE_CLASSES / 2;
        size_class_t* size_class = &global_heap.size_classes[class_index];

        if (!size_class->free_list) {
            void* new_region = mmap_alloc(PAGE_SIZE * 4);
            if (!new_region) {
                pthread_mutex_unlock(&global_heap.lock);
                return NULL;
            }

            block_header_t* new_block = (block_header_t*)new_region;
            new_block->size = PAGE_SIZE * 4 - sizeof(block_header_t);
            new_block->is_free = 1;
            new_block->next = NULL;
            new_block->prev = NULL;

            size_class->free_list = new_block;
        }

        block = size_class->free_list;
        size_class->free_list = block->next;
        block = split_block(block, size_class->size);
    } else {
        size_t alloc_size = size + sizeof(block_header_t);
        void* new_region = mmap_alloc(alloc_size);
        if (!new_region) {
            pthread_mutex_unlock(&global_heap.lock);
            return NULL;
        }

        block = (block_header_t*)new_region;
        block->size = alloc_size - sizeof(block_header_t);
        block->is_free = 0;
        block->next = global_heap.large_blocks;
        block->prev = NULL;

        if (global_heap.large_blocks) {
            global_heap.large_blocks->prev = block;
        }
        global_heap.large_blocks = block;
    }

    block->is_free = 0;
    pthread_mutex_unlock(&global_heap.lock);
    return (void*)((char*)block + sizeof(block_header_t));
}

void free(void* ptr) {
    if (!ptr) {
        return;
    }

    pthread_mutex_lock(&global_heap.lock);

    block_header_t* block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    block->is_free = 1;

    if (block->size > LARGE_THRESHOLD) {
        if (block->prev) {
            block->prev->next = block->next;
        } else {
            global_heap.large_blocks = block->next;
        }
        if (block->next) {
            block->next->prev = block->prev;
        }
        mmap_free(block, block->size + sizeof(block_header_t));
    } else {
        coalesce(block);
        int class_index = block->size <= TINY_MAX_SIZE ? (block->size - 1) / 8 : (block->size - 1) / 256 + NUM_SIZE_CLASSES / 2;
        size_class_t* size_class = &global_heap.size_classes[class_index];
        
        block->next = size_class->free_list;
        block->prev = NULL;
        if (size_class->free_list) {
            size_class->free_list->prev = block;
        }
        size_class->free_list = block;
    }

    pthread_mutex_unlock(&global_heap.lock);
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    block_header_t* block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    if (size <= block->size) {
        return ptr;
    }

    void* new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }

    memcpy(new_ptr, ptr, block->size);
    free(ptr);
    return new_ptr;
}

void* calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void* ptr = malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}
