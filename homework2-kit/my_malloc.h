#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
typedef struct memory
{
    size_t size;
    bool used;
    struct memory* free_mem_next;
    struct memory* free_mem_prev;
} mem;

void * allocate(size_t size, mem* this_mem_part);
mem * get_mem_location(void * data);
mem * split(size_t size, mem * this_mem_part);
void freeList_remove(mem * _next);
void merge(mem *this_mem_part);
void * ff_malloc(size_t size);
void * bf_malloc(size_t size);
void ff_free(void *data);
void bf_free(void *data);
unsigned long get_largest_free_data_segment_size();
unsigned long get_total_free_size();