#ifndef _TEF_HELPERS_H
#define _TEF_HELPERS_H

#define _STRINGIFY_INDIRECT(x) #x
#define _STRINGIFY(x) _STRINGIFY_INDIRECT(x)
#define _FILELINE __FILE__ ":" _STRINGIFY(__LINE__) ": "
#define ERROR(msg) perror("tef error " _FILELINE msg)
#define ERROR_FORMAT(...) fprintf(stderr, "tef error " _FILELINE __VA_ARGS__)

extern void *realloc_safe(void *ptr, size_t size);

#endif
