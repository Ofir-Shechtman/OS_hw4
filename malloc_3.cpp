#include <unistd.h>
#include <cstring>
#include <sys/mman.h>

#define MAX_BLOCK_SIZE 100000000
#define SPLIT_MIN 128
#define MMAP_THRESHOLD (128*1024)

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;

};

static MallocMetadata* _split(MallocMetadata *pMetadata, size_t size);
static void _merge_adj_frees(MallocMetadata *pMetadata);

static MallocMetadata *
_merge_blocks(MallocMetadata *first, MallocMetadata *second);

static MallocMetadata* block_head= nullptr;
static MallocMetadata* block_tail= nullptr;
static MallocMetadata* mmap_head= nullptr;

size_t _size_meta_data() {return sizeof(MallocMetadata);}



void* _enlarged_wilderness(size_t size) {
    void *p = sbrk(size - block_tail->size);
    if (*(int *) p == -1)
        return nullptr;
    block_tail->size=size;
    block_tail->is_free=false;
    return (MallocMetadata*)block_tail+ 1;
}

void * smalloc(size_t size) {
    if(size==0 || size > MAX_BLOCK_SIZE)
        return nullptr;
    if(size>=MMAP_THRESHOLD){
        void* p = mmap(NULL, size+ _size_meta_data(), PROT_READ | PROT_WRITE , MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        auto new_map =(MallocMetadata*) p;
        new_map->size=size;
        new_map->is_free=false;
        new_map->prev= nullptr;
        new_map->next=mmap_head;
        if(mmap_head) mmap_head->prev=new_map;
        mmap_head=new_map;
        return (MallocMetadata*)p + 1;
    }

    MallocMetadata* curr= block_head;
    while(curr){
        if(curr->is_free && curr->size >= size){
            _split(curr, size);
            return (MallocMetadata*) curr + 1;
        }
        curr=curr->next;

    }
    if(block_tail && block_tail->is_free) { //challenge 3
        return _enlarged_wilderness(size);
    }

    void* p = sbrk(size+ _size_meta_data());
    if(*(int*)p==-1)
        return nullptr;
    auto new_node =(MallocMetadata*) p;//TODO: check if works
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

static MallocMetadata* _split(MallocMetadata *curr, size_t size) {
    curr->is_free=false;
    if(curr->size-size>=SPLIT_MIN+_size_meta_data()){
        auto new_split = (MallocMetadata*) ((char*)curr + size) +1;
        new_split->size = curr->size-size-_size_meta_data();
        new_split->is_free=true;
        new_split->prev=curr;
        new_split->next=curr->next;
        if(curr->next) curr->next->prev=new_split;
        curr->next=new_split;
        curr->size=size;
        if(curr == block_tail)
            block_tail=new_split;
        if(new_split->next && new_split->next->is_free)
            _merge_blocks(new_split, new_split->next);
        return new_split;
    }
    else {
        return curr;
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
    auto* metadata = (MallocMetadata*)p-1; //TODO: check if works, try while
    if(metadata->is_free)
        return;
    if(metadata->size>MMAP_THRESHOLD){
        if(metadata==mmap_head) mmap_head=metadata->next;
        if(metadata->prev) metadata->prev->next = metadata->next;
        if(metadata->next) metadata->next->prev = metadata->prev;
        munmap(metadata, metadata->size+_size_meta_data());
        return;
    }
    metadata->is_free=true;
    _merge_adj_frees(metadata);
}

static void _merge_adj_frees(MallocMetadata *pMetadata) {
    if(pMetadata->prev && pMetadata->prev->is_free)
        pMetadata= _merge_blocks(pMetadata->prev, pMetadata);

    if(pMetadata->next && pMetadata->next->is_free)
        _merge_blocks(pMetadata, pMetadata->next);

}

static MallocMetadata * _merge_blocks(MallocMetadata *first, MallocMetadata *second) {
    first->size += second->size + _size_meta_data();
    if(second->next) second->next->prev=first;
    first->next=second->next;
    if(second == block_tail)
        block_tail=first;
    return first;
}

void* srealloc(void* oldp, size_t size){
    if(size==0 || size > 100000000)
        return nullptr;
    if(oldp) {
        auto *metadata = (MallocMetadata *) oldp -1;
        if(metadata->is_free)
            return smalloc(size);
        if (metadata->size >= size) {//a
            if(size < MMAP_THRESHOLD)
                _split(metadata, size);
            else
                metadata->size=size;
            return oldp;
        }
        if(metadata == block_tail) //comment 3
            return _enlarged_wilderness(size);

        if (size < MMAP_THRESHOLD) {
            if (metadata->prev && metadata->prev->is_free && metadata->size +
                metadata->prev->size + _size_meta_data() >= size) { //b
                _merge_blocks(metadata->prev, metadata);
                std::memcpy( metadata->prev + 1, metadata + 1, metadata->size);
                metadata->prev->is_free = false;
                _split(metadata->prev, size);
                return metadata->prev + 1;
            }

            if (metadata->next && metadata->next->is_free && metadata->size +
                metadata->next->size + _size_meta_data() >= size) { //c
                _merge_blocks(metadata, metadata->next);
                _split(metadata, size);
                return metadata + 1;
            }

            if (metadata->prev && metadata->next && metadata->next->is_free &&
                metadata->prev->is_free && metadata->size +
                metadata->prev->size + metadata->next->size + _size_meta_data()*2 >= size) { //d
                MallocMetadata *first = _merge_blocks(metadata->prev,
                                                      metadata);
                _merge_blocks(first, metadata->next);
                std::memcpy( metadata->prev + 1, metadata + 1, metadata->size);
                first->is_free = false;
                _split(first, size);
                return first + 1;
            }
        }
    }
    void* new_alloc= smalloc(size); //e&f or mmap
    if(!new_alloc)
        return nullptr;
    if(oldp) {
        std::memcpy(new_alloc, oldp, size);
        sfree(oldp);
    }
    return new_alloc;
}

MallocMetadata* _get_block_head(){
    return block_head;
}

MallocMetadata* _get_block_tail(){
    return block_tail;
}

MallocMetadata* _get_mmap_head(){
    return mmap_head;
}

MallocMetadata* _get_mmap_tail(){
    if(!mmap_head) return nullptr;
    auto curr=mmap_head;
    while(curr->next)
        curr=curr->next;
    return curr;
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
    curr=mmap_head;
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
    curr=mmap_head;
    while(curr){
        counter+=curr->size;
        curr = curr->next;
    }
    return counter;
}

size_t _num_meta_data_bytes() {
    return _size_meta_data()*_num_allocated_blocks();
}