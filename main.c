#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>


#define PAGE_SIZE 4096
static int i=0;
static void* c1= NULL;


void handler(int sig, siginfo_t *dont_care, void *dont_care_either)
{
    static int fault=0;
    fault++;
    void* page_fault = ((char*) c1 + i);
    printf("fault #%d in %p (%p+%d)\n",fault,  page_fault, c1, i);
    //void* c1 = sbrk(0);
    //printf("program break address: %p\n", c1);
    //printf("%d\n", i);
    //sbrk(PAGE_SIZE);
    //c1 = sbrk(0);
    //printf("program break address: %p\n", c1);
    //i=0;

    if(fault>8)
        exit(1);
}

int main()
{
    struct sigaction sa;
    sa.sa_flags     = SA_NODEFER;
    sa.sa_sigaction = handler;
    sigaction(SIGSEGV, &sa, NULL); /* ignore whether it works or not */
    assert(getpagesize()==PAGE_SIZE);
    c1 = sbrk(0);
    printf("My page size: %d\n", PAGE_SIZE);
    printf("program break address: %p\n", c1);
    //c1 = (void*) ((char*) c1 + 1);
    //printf("c1: %p\n", c1);
    void* p = sbrk(-10000000000000000000000000);
    printf("program break address: %p\n", p);
    void* c2 = malloc(PAGE_SIZE*200);
    *(char*)c2=0;
    printf("malloc address: %p\n", c2);
    printf("diff: %d\n", c2-c1);
    void* c3 = sbrk(0);
    printf("program break address: %p\n", c1);
    printf("diff: %d\n", c3-c1);
    for(i=0; i<PAGE_SIZE*200; ++i){
        *(int*)(c1+i)=0;
    }

}