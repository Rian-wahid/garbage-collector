#ifndef GC_H
#include<stddef.h>
#define GC_H
// free unused allocated memory
void clean();
// free all allocated memory
void clear();
// allocate memory using malloc
void alloc_mem(void **const ptr_to_ptr,size_t size);
// assign a pointer
void assign_ptr(void **const ptr_to_ptr,void *ptr);
// force garbage collector to deallocate pointer
void force_free(void *ptr);
#endif

