// mem_tracking.h

 // Author: tommojphillips
 // GitHub: https:\\github.com\tommojphillips

#define MEM_TRACKING
#ifdef NO_MEM_TRACKING
#undef MEM_TRACKING
#endif

#ifndef MEM_TRACKING_H
#define MEM_TRACKING_H
#ifdef MEM_TRACKING

#include <stdio.h>

extern int memtrack_allocations;
extern long memtrack_allocatedBytes;

int memtrack_report();
void* memtrack_malloc(size_t size);
void memtrack_free(void* ptr);
void* memtrack_realloc(void* ptr, size_t size);
void* memtrack_calloc(size_t count, size_t size);

#define malloc(size) memtrack_malloc(size)
#define free(ptr) memtrack_free(ptr)
#define realloc(ptr, size) memtrack_realloc(ptr, size)
#define calloc(count, size) memtrack_calloc(count, size)

#endif // !MEM_TRACKING
#endif // !MEM_TRACKING_H
