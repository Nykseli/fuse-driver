#ifndef FSALLOC_H
#define FSALLOC_H

#include <stddef.h>

void* fsalloc(size_t size);
void* fscalloc(size_t nmemb, size_t size);
void fsfree(void* mem);
void* fsrealloc(void* mem, size_t size);

#undef malloc
#define malloc fsalloc
#undef calloc
#define calloc fscalloc
#undef free
#define free fsfree
#undef realloc
#define realloc fsrealloc

#endif
