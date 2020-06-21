#include <unistd.h>

void* smalloc(size_t size){
    if(size==0 || size > 100000000)
        return NULL;
    void* p = sbrk(size);
    if(*(int*)p==-1)
        return NULL;
    return p;
}