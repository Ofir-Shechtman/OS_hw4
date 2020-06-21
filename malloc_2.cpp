#include <unistd.h>
#include <cstring>

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;

};

static MallocMetadata* head= NULL;
static size_t num_free_blocks=0;
static size_t num_alloc_blocks=0;
static size_t num_free_bytes=0;
static size_t num_alloc_bytes=0;


size_t _num_free_blocks() {return num_free_blocks;}
size_t _num_of_free_bytes() {return num_free_bytes;}
size_t _num_allocated_blocks() {return num_alloc_blocks;}
size_t _num_of_allocated_bytes() {return num_alloc_bytes;}
size_t _size_meta_data() {return sizeof(MallocMetadata);}
size_t _num_metadata_bytes() {
    return _size_meta_data()*_num_allocated_blocks();
}



void * smalloc(size_t size) {
    if(size==0 || size > 100000000)
        return NULL;
    MallocMetadata* curr= head;
    MallocMetadata* tail=NULL;
    while(curr){
        if(curr->is_free && curr->size >= size){
            curr->is_free=false;
            num_free_blocks--;
            num_free_bytes-=curr->size;
            return (char*) curr + _size_meta_data();
        }
        if(!curr->next)
            tail=curr;
        curr=curr->next;

    }
    void* p = sbrk(size+ _size_meta_data());
    if(*(int*)p==-1)
        return NULL;
    auto* new_node =(MallocMetadata*) p;//TODO: check if works
    num_alloc_blocks++;
    num_alloc_bytes+=size;
    if(!head)
        head=new_node;
    new_node->size=size;
    new_node->is_free=false;
    new_node->prev=tail;
    new_node->next=NULL;
    if(tail)
        tail->next=new_node;

    return (char*)p + _size_meta_data();;

}


void* scalloc(size_t num, size_t size){
    void* p= smalloc(num*size);
    if(p)
        std::memset(p, 0, num*size);
    return p;

}
void* sfree(void* p);
void* srealloc(void* oldp, size_t size);

/*
class Heap{
public:
    static Heap& getInstance(){
        static Heap instance;
        return instance;
    }
    Heap(Heap const&)           = delete;
    void operator=(Heap const&) = delete;
    static void* smalloc(size_t size);
    void* scalloc(size_t num, size_t size);
    void* sfree(void* p);
    void* srealloc(void* oldp, size_t size);
    size_t get_num_of_free_blocks() {return num_free_blocks;};           //5
    size_t get_num_of_alloc_blocks() {return num_alloc_blocks;};         //6
    size_t get_num_of_free_bytes() {return num_free_bytes;};             //7
    size_t get_num_of_alloc_bytes() {return num_alloc_bytes;};           //8
    size_t get_num_metadata_bytes() {                                       //9
        return _size_meta_data()*get_num_of_alloc_blocks();
    };
    static size_t _size_meta_data() {return sizeof(MallocMetadata);};

private:
    MallocMetadata* head;
    size_t num_free_blocks;
    size_t num_alloc_blocks;
    size_t num_free_bytes;
    size_t num_alloc_bytes;
    Heap(): head(nullptr) {}

};


void* smalloc(size_t size){
    return Heap::getInstance().smalloc(size);
}
void* scalloc(size_t num, size_t size){
    return Heap::getInstance().scalloc(num, size);
}
void* sfree(void* p){
    return Heap::getInstance().sfree(p);
}
void* srealloc(void* oldp, size_t size){
    return Heap::getInstance().srealloc(oldp, size);
}
size_t _num_free_blocks() {

}
size_t get_num_of_alloc_blocks() {return num_alloc_blocks;};         //6
size_t get_num_of_free_bytes() {return num_free_bytes;};             //7
size_t get_num_of_alloc_bytes() {return num_alloc_bytes;};           //8
size_t get_num_metadata_bytes() {                                       //9

    */