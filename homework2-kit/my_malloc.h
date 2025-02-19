#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

typedef struct memory
{
    size_t size;
    struct memory* free_mem_next;
    struct memory* free_mem_prev;
    struct memory* thread_next;
    struct memory* thread_prev;
    // 动态切换的指针
    struct memory* global_next;
    struct memory* global_prev;

} mem;

void *ts_malloc_lock(size_t size);
void ts_free_lock(void *p);
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *p);
void * bf_malloc(size_t size, bool use_unlock);
void * this_allocate(size_t size, bool use_unlock);
void * this_malloc(size_t size, mem* ptr, bool use_unlock);
void this_remove(mem * this, bool use_unlock);
mem * split(size_t size, mem * ptr);
mem * get_mem_location(void * data);
void bf_free(void *data, bool use_unlock);
mem * add(mem * curr, mem * prev, mem* next, bool use_unlock);
void merge(mem *prev, mem* curr) ;