#include <mymalloc.h>


#define CHUNK_SIZE sizeof(chunk_t)
#define min_block 8

typedef struct chunk {
	size_t size;
//	int  is_free;
//	struct chunk *prev;
	struct chunk *next;
} chunk_t;

typedef struct block{
	size_t size;
//	struct block *prev;
	struct block *next;
}block_t;

spinlock_t big_lock;

static chunk_t *my_chunk=NULL;
static block_t *my_block=NULL;

// We don't need this malloc_count. You can remove it.
long malloc_count;


static inline size_t _align_up_bytes(size_t size){
	return (size + 7) & ~7;
}

static inline size_t _align_up_page(size_t size){
	return (size + 4095) & ~4095;
}

static chunk_t* allocate_chunk(size_t size){
	size_t align_size = _align_up_page(size);

	my_chunk =  vmalloc(NULL,align_size);

	if(my_chunk==NULL)
		return NULL;

	my_chunk->size = align_size;
//	my_chunk->is_free=1;
//	my_chunk->prev =NULL;
	my_chunk->next = NULL;

	return my_chunk;
}

static block_t * allocate_block( ){
//	size_t align_size = _align_up_bytes(size);

//	chunk_t* my_block =  vmalloc(NULL,align_size);

	if(my_block==NULL)
		return NULL;

	my_block->size = my_chunk->size;
//	my_chunk->is_free=1;
//	my_chunk->prev =NULL;
	my_block->next = NULL;

	return my_block;
}


void *mymalloc(size_t size) {
	if (size==0)  return NULL;
	size_t request_size = _align_up_bytes(size);

    spin_lock(&big_lock);

	if(my_chunk==NULL){
		my_chunk =  allocate_chunk(request_size);
//		spin_unlock(&big_lock);
		if (my_chunk==NULL){
			return NULL;
		}
//		return ;
	}else{
		if(my_block==NULL){
			my_block = allocate_block();
			block_t * a;
			a->next=NULL;
			a->size= my_block->size - sizeof (block_t);
			my_block->next=a;



			size_t res_size = a->size;
			size_t total_size = request_size + sizeof (block_t);

			if(res_size>= total_size){
				if(res_size - total_size >= sizeof(block_t)+ min_block){
					block_t * b = (block_t*)(request_size+ sizeof (block_t));
					b->size = res_size - total_size;
					a->next=b;
				}
			}

			if(res_size - total_size<= sizeof(block_t)+ min_block){
				al_total_size = _align_up_bytes(total_size)
				my_chunk =  allocate_chunk(total_size);
				if (my_chunk==NULL){
					return NULL;
				}

				my_block = allocate_block();
				block_t * a;
				a->next=NULL;
				a->size= my_block->size - sizeof (block_t);
				my_block->next=a;
			}


		}else{
			while (my_block!=NULL){
				size_t res_size = my_block->size;
				size_t total_size = request_size + sizeof (block_t);

				if(res_size>= total_size){
					if(res_size - total_size >= sizeof(block_t)+ min_block){
						block_t * b = (block_t*)(request_size+ sizeof (block_t));
						b->size = res_size - total_size;
						my_blocka->next=b;
					}
				}

				my_block=my_block->next;
			}
		}


	}




    spin_unlock(&big_lock);

    return NULL;
}

void myfree(void *ptr) {

}
