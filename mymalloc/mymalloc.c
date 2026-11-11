#include <mymalloc.h>


#define CHUNK_SIZE sizeof(chunk_t)

typedef struct chunk {
	size_t size;
	int  is_free;
	struct chunk *prev;
	struct chunk *next;
} chunk_t;

typedef struct block{
	size_t size;
	struct block *prev;
	struct block *next;
}block_t;

spinlock_t big_lock;

static chunk_t *my_heap=NULL;

// We don't need this malloc_count. You can remove it.
long malloc_count;


static inline size_t _align_up_bytes(size_t size){
	return (size + 7) & ~7;
}

static inline size_t _align_up_page(size_t size){
	return (size + 4095) & ~4095;
}

static chunk_t* allocate_heap(size_t size){
	size_t align_size = _align_up_page(size);

	chunk_t* my_heap =  vmalloc(NULL,align_size);

	if(my_heap==NULL)
		return NULL;

	my_heap->size = align_size;
	my_heap->is_free=1;
	my_heap->prev =NULL;
	my_heap->next = NULL;

	return my_heap;
}


void *mymalloc(size_t size) {
	if (size==0)  return NULL;
	size_t align_size = _align_up_bytes(size);

    spin_lock(&big_lock);

	if(my_heap==NULL){
		chunk_t *my_heap =  allocate_heap(align);
//		spin_unlock(&big_lock);
		if (my_heap==NULL){
			return NULL;
		}
//		return ;
	}else{
		chunk_t *heap_head = my_heap;
		while (heap_head!=NULL){
			int free =  heap_head->is_free;
			size_t res_size = heap_head->size;
			if((free) && res_size>=align_size){

			}


			heap_head=heap_head->next;
		}

	}




    spin_unlock(&big_lock);

    return NULL;
}

void myfree(void *ptr) {

}
