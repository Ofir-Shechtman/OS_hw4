#include <cassert>
#include <cstdio>
#include "malloc_2.h"


int main(){
    void* p = smalloc(10);
    void* c1 = sbrk(0);
    printf("program break address: %p\n", c1);
    printf("program break address: %p\n", p);
    printf("diff: %ld\n", (char*)p-(char*)c1);
    assert((char*)p-(char*)c1==-10);


}