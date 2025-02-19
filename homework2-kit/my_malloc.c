#include "my_malloc.h"
#include "assert.h"
#include <pthread.h>
// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wdeprecated-declarations"
mem **freeStart = NULL;
mem *freeEnd = NULL;//没有用？
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 定义一个锁
__thread mem **thread_freeStart = NULL;//每个线程都有自己的空闲链表
void *ts_malloc_lock(size_t size) {
    pthread_mutex_lock(&mutex);
    void *p = bf_malloc(size);
    pthread_mutex_unlock(&mutex);
    return p;
}
void ts_free_lock(void *p) {
    pthread_mutex_lock(&mutex);
    bf_free(p);
    pthread_mutex_unlock(&mutex);
}

// void *ts_malloc_nolock(size_t size) {
//     void *p;
//     p = bf_malloc(size);
//     if (p == NULL) {
//         pthread_mutex_lock(&mutex);
//         p = sbrk(size);
//         pthread_mutex_unlock(&mutex);
//     }
//     return p;
// }

// void ts_free_nolock(void *p) {
//     bf_free(p);
// }
void * bf_malloc(size_t size){
    if (size == 0) {
        return NULL; 
    }
    mem* free_p = freeStart;
    mem* best = NULL;
    while (free_p != NULL) {
        if(free_p->size >= size){
            if(best->size == size){
                return malloc(size, best);
            }
            if(best == NULL || free_p->size < best->size){
                best = free_p;
            }
        }
        free_p = free_p->free_mem_next;
    }
    if(best->size){
        return malloc(size, best);
    }
    return allocate(size);
}
void * allocate(size_t size){
    size_t mem_size = size + sizeof(mem);
    mem *new = sbrk(mem_size);
    if (new == (void *)-1) {
        perror("sbrk failed");
        return NULL; // out of memory
    }
    new->free_mem_next = NULL;
    new->size = size;
    new->free_mem_prev = NULL;
    return ((char*)new+sizeof(mem));//返回新mem被使用的地址，mem地址为&new
}
void * malloc(size_t size, mem* this){
    remove(this);
    if(this->size <= size + sizeof(mem)){
        return ((char*)this+sizeof(mem));//返回新mem被使用的地址
    }
    mem* new = split(size, this);
    return ((char*)new+sizeof(mem));
}
void remove(mem * this){
    //assert(_next && freeEnd);
    if(this->free_mem_prev ){
        //printf("_next->free_mem_prev ");
        this->free_mem_prev->free_mem_next = this->free_mem_next;
    }
    if(this->free_mem_next){
        //printf("_next->free_mem_next ");
        this->free_mem_next->free_mem_prev = this->free_mem_prev;
    }
    freeStart = this->free_mem_prev;
    this->free_mem_next = NULL;
    this->free_mem_prev = NULL;
}
mem * split(size_t size, mem * this){
    assert(this && this->size> size+sizeof(mem));
    if (this == NULL || this->size <= (size + sizeof(mem))) {
        return NULL;
    }
    //把后面的划走 use
    mem * new_used = (mem*)((char *)this + this->size - size);
    new_used->size = size;
    new_used->free_mem_prev = NULL;
    new_used->free_mem_next = NULL;
    this->size -= sizeof(mem) + size;
    return new_used;
}
mem * get_mem_location(void * data){
    return (mem*)((char *)data - sizeof(mem));
}

void bf_free(void *data) {
    if (data == NULL) {
        return;
    }
    mem* curr = (mem*)((char *)data - sizeof(mem));
    if (curr == NULL) {
        //printf("Invalid pointer\n");
        return;
    }
    mem* next = freeStart;
    mem* prev = NULL;
    while (next && next < curr) {
        prev = next;
        next = next->free_mem_next;
    }
    curr = add(curr, prev, next);
    if(prev && (char*)prev +sizeof(mem)+ prev->size == (char*)curr){
        merge(prev, curr);
    }
    if(next && (char*)curr +sizeof(mem)+ curr->size == (char*)next){
        merge(curr, next);
    }
}
mem * add(mem * curr, mem * prev, mem* next){
    curr->free_mem_prev = prev;
    curr->free_mem_next = next;
    if(prev == NULL){
        freeStart = curr;
    }
    else if(next == NULL){
        prev->free_mem_next = curr;
    }
    return(curr);
}

void merge(mem *prev, mem* curr) {
    assert(curr && curr->free_mem_prev);
    if (curr == NULL) {
        //printf("Error: NULL pointer passed to merge\n");
        return;
    }
    prev->free_mem_next = curr->free_mem_next;
    curr->free_mem_prev = NULL;
    prev->size += curr->size +sizeof(mem); 
    if(curr->free_mem_next == NULL){
        curr->free_mem_next = NULL;
        return;
    }
    curr->free_mem_next->free_mem_prev = prev;
}


