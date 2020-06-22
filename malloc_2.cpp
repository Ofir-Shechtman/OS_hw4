#include <unistd.h>
#include <cstring>

#define MAX_BLOCK_SIZE 100000000

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;

};



static MallocMetadata* block_head= nullptr;
static MallocMetadata* block_tail= nullptr;


size_t _size_meta_data() {return sizeof(MallocMetadata);}




void * smalloc(size_t size) {
    if(size==0 || size > MAX_BLOCK_SIZE)
        return nullptr;
    MallocMetadata* curr= block_head;
    while(curr){
        if(curr->is_free && curr->size >= size){
            curr->is_free=false;
            return (MallocMetadata*) curr + 1;
        }
        curr=curr->next;

    }
    void* p = sbrk(size+ _size_meta_data());
    if(*(int*)p==-1)
        return nullptr;
    auto new_node =(MallocMetadata*) p;
    if(!block_head)
        block_head=new_node;
    new_node->size=size;
    new_node->is_free=false;
    new_node->prev=block_tail;
    new_node->next=nullptr;
    if(block_tail)
        block_tail->next=new_node;
    block_tail=new_node;

    return (MallocMetadata*)p + 1;

}


void* scalloc(size_t num, size_t size){
    void* p= smalloc(num*size);
    if(p)
        std::memset(p, 0, num*size);
    return p;

}
void sfree(void* p){
    if(!p) return;
    auto* metadata = (MallocMetadata*)p-1;
    metadata->is_free=true;
}

void* srealloc(void* oldp, size_t size){
    if(size==0 || size > MAX_BLOCK_SIZE)
        return nullptr;
    if(!oldp)
        return smalloc(size);

    auto *metadata = (MallocMetadata *) oldp -1;
    if(metadata->size>=size)
        return oldp;
    void* new_alloc= smalloc(size);
    if(!new_alloc)
        return nullptr;

    std::memcpy(new_alloc, oldp, size);
    sfree(oldp);
    return new_alloc;
}

MallocMetadata* _get_block_head(){
    return block_head;
}

MallocMetadata* _get_block_tail(){
    return block_tail;
}


size_t _num_free_blocks() {
    unsigned int counter=0;
    auto curr=block_head;
    while(curr) {
        if (curr->is_free)
            counter++;
        curr = curr->next;
    }
    return counter;
}
size_t _num_free_bytes() {
    unsigned int counter=0;
    auto curr=block_head;
    while(curr) {
        if (curr->is_free)
            counter += curr->size;
        curr = curr->next;
    }
    return counter;
}
size_t _num_allocated_blocks() {
    unsigned int counter=0;
    auto curr=block_head;
    while(curr){
        counter++;
        curr = curr->next;
    }
    return counter;
}
size_t _num_allocated_bytes() {
    unsigned int counter=0;
    auto curr=block_head;
    while(curr){
        counter+=curr->size;
        curr = curr->next;
    }
    return counter;
}

size_t _num_meta_data_bytes() {
    return _size_meta_data()*_num_allocated_blocks();
}