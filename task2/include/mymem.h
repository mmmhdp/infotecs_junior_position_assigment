#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Core C functions
void *my_malloc(size_t size);
void my_free(void *ptr);

#ifdef __cplusplus
}
#endif

// Optional macro replacement
#ifdef USE_CUSTOM_ALLOCATOR
#define malloc(size) my_malloc(size)
#define free(ptr) my_free(ptr)
#endif

