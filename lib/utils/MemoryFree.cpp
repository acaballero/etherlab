#include <sys/unistd.h>
extern char *__bss_end__;
extern char *__heap_limit__;

#include "MemoryFree.h"

int freeMemory() {

    char *heap_end = (char *)sbrk(0);
    return (heap_end - __heap_limit__);
}
