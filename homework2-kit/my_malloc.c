#include "my_malloc.h"
#include "assert.h"
#include <pthread.h>
#include <stdbool.h>

// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wdeprecated-declarations"
mem *freeStart = NULL;
__thread mem *thread_freeStart = NULL;//每个线程都有自己的空闲链表
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 定义一个锁
void *ts_malloc_lock(size_t size) {
    pthread_mutex_lock(&mutex);
    mem *free_list = freeStart;
    while (free_list) {
        free_list->global_next = free_list->free_mem_next;
        free_list->global_prev = free_list->free_mem_prev;
        free_list = free_list->free_mem_next;
    }
    void *p = bf_malloc(size, false);
    free_list = freeStart;
    while(free_list){
        freeStart->free_mem_next = freeStart->global_next;
        freeStart->free_mem_prev = freeStart->global_prev;
        free_list = free_list->global_next;
    }
    pthread_mutex_unlock(&mutex);
    return p;
}
void ts_free_lock(void *p) {
    pthread_mutex_lock(&mutex);
    mem *free_list = freeStart;
    while (free_list) {
        free_list->global_next = free_list->free_mem_next;
        free_list->global_prev = free_list->free_mem_prev;
        free_list = free_list->free_mem_next;
    }
    bf_free(p, false);
    free_list = freeStart;
    while(free_list){
        freeStart->free_mem_next = freeStart->global_next;
        freeStart->free_mem_prev = freeStart->global_prev;
        free_list = free_list->global_next;
    }
    pthread_mutex_unlock(&mutex);
}

void *ts_malloc_nolock(size_t size) {
    mem *free_list = thread_freeStart;
    while (free_list) {
        free_list->global_next = free_list->thread_next;
        free_list->global_prev = free_list->thread_prev;
        free_list = free_list->thread_next;
    }
    void *p = bf_malloc(size, true);
    if (p == NULL) {
        pthread_mutex_lock(&mutex);
        p = sbrk(size);
        pthread_mutex_unlock(&mutex);
    }
    return p;
    free_list = thread_freeStart;
    while(free_list){
        freeStart->thread_next = freeStart->global_next;
        freeStart->thread_prev = freeStart->global_prev;
        free_list = free_list->global_next;
    }
}

void ts_free_nolock(void *p) {
    mem *free_list = thread_freeStart;
    while (free_list) {
        free_list->global_next = free_list->thread_next;
        free_list->global_prev = free_list->thread_prev;
        free_list = free_list->thread_next;
    }
    bf_free(p, true);
    free_list = thread_freeStart;
    while(free_list){
        freeStart->thread_next = freeStart->global_next;
        freeStart->thread_prev = freeStart->global_prev;
        free_list = free_list->global_next;
    }
}
void * bf_malloc(size_t size, bool use_unlock){
    if (size == 0) {
        return NULL; 
    }
    pthread_mutex_lock(&mutex); //lock 1
    mem* free_p = freeStart;
    mem* best = NULL;
    while (free_p != NULL) {
        if(free_p->size >= size){
            if(best == NULL || (free_p->size < best->size && best->size>size) ){
                best = free_p;
            }
            else if(best->size == size){
                return this_malloc(size, best, use_unlock);
            }
        }
        free_p = free_p->global_next;
    }
    if(best){
        if(best->size >= size){
            pthread_mutex_unlock(&mutex); 
            return this_malloc(size, best, use_unlock);}
    }
    if(use_unlock){
        pthread_mutex_unlock(&mutex); 
        return NULL;
    }
    pthread_mutex_unlock(&mutex); 
    return this_allocate(size, use_unlock);
}
void * this_allocate(size_t size, bool use_unlock){
    size_t mem_size = size + sizeof(mem);
    mem *new = sbrk(mem_size);
    if (new == (void *)-1) {
        perror("sbrk failed");
        return NULL; // out of memory
    }
    new->global_next = NULL;
    new->size = size;
    new->global_prev = NULL;
    return ((char*)new+sizeof(mem));//返回新mem被使用的地址，mem地址为&new
}
void * this_malloc(size_t size, mem* ptr, bool use_unlock){
    this_remove(ptr, use_unlock);
    if(ptr->size <= size + sizeof(mem)){
        return ((char*)ptr+sizeof(mem));//返回新mem被使用的地址
    }
    mem* new = split(size, ptr);
    return ((char*)new+sizeof(mem));
}
void this_remove(mem * ptr, bool use_unlock){
    //assert(_next && freeEnd);
    if(ptr->global_prev ){
        //printf("_next->global_prev ");
        ptr->global_prev->global_next = ptr->global_next;
    }
    if(ptr->global_next){
        //printf("_next->global_next ");
        ptr->global_next->global_prev = ptr->global_prev;
    }
    if(use_unlock){thread_freeStart = ptr->global_prev;}
    else{freeStart = ptr->global_prev;}
    ptr->global_next = NULL;
    ptr->global_prev = NULL;
}
mem * split(size_t size, mem * ptr){
    assert(ptr && ptr->size> size+sizeof(mem));
    if (ptr == NULL || ptr->size <= (size + sizeof(mem))) {
        return NULL;
    }
    //把后面的划走 use
    mem * new_used = (mem*)((char *)ptr + ptr->size - size);
    new_used->size = size;
    new_used->global_prev = NULL;
    new_used->global_next = NULL;
    ptr->size -= sizeof(mem) + size;
    return new_used;
}
mem * get_mem_location(void * data){
    return (mem*)((char *)data - sizeof(mem));
}

void bf_free(void *data, bool use_unlock) {
    if (data == NULL) {
        return;
    }
    mem* curr = get_mem_location(data);
    if (curr == NULL) {
        //printf("Invalid pointer\n");
        return;
    }
    mem* next;
    if(use_unlock){ next = thread_freeStart;}
    else{next = freeStart;}
    mem* prev = NULL;
    while (next && next < curr) {
        prev = next;
        next = next->global_next;
    }
    curr = add(curr, prev, next, use_unlock);
    if(prev && (char*)prev +sizeof(mem)+ prev->size == (char*)curr){
        merge(prev, curr);
    }
    if(next && (char*)curr +sizeof(mem)+ curr->size == (char*)next){
        merge(curr, next);
    }
}
mem * add(mem * curr, mem * prev, mem* next, bool use_unlock){
    curr->global_prev = prev;
    curr->global_next = next;
    if(prev == NULL){
        if(use_unlock){thread_freeStart = curr;}
        else{freeStart = curr;}
    }
    else if(next == NULL){
        prev->global_next = curr;
    }
    return(curr);
}

void merge(mem *prev, mem* curr) {
    assert(curr && curr->global_prev);
    if (curr == NULL) {
        //printf("Error: NULL pointer passed to merge\n");
        return;
    }
    prev->global_next = curr->global_next;
    curr->global_prev = NULL;
    prev->size += curr->size +sizeof(mem); 
    if(curr->global_next == NULL){
        curr->global_next = NULL;
        return;
    }
    curr->global_next->global_prev = prev;
}


