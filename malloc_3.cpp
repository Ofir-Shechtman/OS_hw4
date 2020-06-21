#include <unistd.h>
#include <cstring>

#define SPLIT_MIN 128

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;

};

void _split(MallocMetadata *pMetadata, size_t size);

static MallocMetadata* head= nullptr;
static size_t num_free_blocks=0;
static size_t num_alloc_blocks=0;
static size_t num_free_bytes=0;
static size_t num_alloc_bytes=0;


size_t _num_free_blocks() {return num_free_blocks;}
size_t _num_free_bytes() {return num_free_bytes;}
size_t _num_allocated_blocks() {return num_alloc_blocks;}
size_t _num_allocated_bytes() {return num_alloc_bytes;}
size_t _size_meta_data() {return sizeof(MallocMetadata);}
size_t _num_meta_data_bytes() {
    return _size_meta_data()*_num_allocated_blocks();
}



void * smalloc(size_t size) {
    if(size==0 || size > 100000000)
        return nullptr;
    MallocMetadata* curr= head;
    MallocMetadata* tail=nullptr;
    while(curr){
        if(curr->is_free && curr->size >= size){
            _split(curr, size);
            return (char*) curr + _size_meta_data();
        }
        if(!curr->next)
            tail=curr;
        curr=curr->next;

    }
    void* p = sbrk(size+ _size_meta_data());
    if(*(int*)p==-1)
        return nullptr;
    auto new_node =(MallocMetadata*) p;//TODO: check if works
    num_alloc_blocks++;
    num_alloc_bytes+=size;
    if(!head)
        head=new_node;
    new_node->size=size;
    new_node->is_free=false;
    new_node->prev=tail;
    new_node->next=nullptr;
    if(tail)
        tail->next=new_node;

    return (char*)p + _size_meta_data();

}

void _split(MallocMetadata *curr, size_t size) {
    curr->is_free=false;
    if(curr->size-size>=SPLIT_MIN+_size_meta_data()){
        auto new_split = (MallocMetadata*) ((char*)curr + size);
        new_split->size = curr->size-size-_size_meta_data();
        new_split->is_free=true;
        new_split->prev=curr;
        new_split->next=curr->next;
        if(curr->next) curr->next->prev=new_split;
        curr->next=new_split;
        curr->size=size;
        num_free_bytes -= (size+_size_meta_data());
    }
    else {
        num_free_blocks--;
        num_free_bytes-=curr->size;
    }
}


void* scalloc(size_t num, size_t size){
    void* p= smalloc(num*size);
    if(p)
        std::memset(p, 0, num*size);
    return p;

}
void sfree(void* p){
    if(!p) return;
    auto* metadata = (MallocMetadata*) ((char*) p - _size_meta_data()); //TODO: check if works, try while
    metadata->is_free=true;
    num_free_blocks++;
    num_free_bytes+=metadata->size;
}

void* srealloc(void* oldp, size_t size){
    if(size==0 || size > 100000000)
        return nullptr;
    auto* metadata = (MallocMetadata*) ((char*) oldp - _size_meta_data());
    if(metadata->size>=size)
        return oldp;
    void* new_aloc= smalloc(size);
    if(!new_aloc)
        return nullptr;
    if(oldp) {
        std::memcpy(new_aloc, oldp, size);
        sfree(oldp);
    }
    return new_aloc;
}

MallocMetadata* _get_head(){
    return head;
}

MallocMetadata* _get_tail(){
    MallocMetadata* curr=head;
    if(!curr) return nullptr;
    while(curr->next){
        curr=curr->next;
    }
    return curr;
}
