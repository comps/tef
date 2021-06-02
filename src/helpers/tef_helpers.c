#include <stdlib.h>

// free the block if realloc fails
void *realloc_safe(void *ptr, size_t size)
{
    void *new = realloc(ptr, size);
    if (new == NULL)
        free(ptr);
    return new;
}
