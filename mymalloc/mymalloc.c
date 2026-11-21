#include <mymalloc.h>

#define CHUNK_SIZE sizeof(chunk_t)
#define BLOCK_SIZE sizeof(block_t)

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
	int is_free;
	struct block *next;
}block_t;

spinlock_t big_lock;

static chunk_t *my_chunk=NULL;
static block_t *free_list=NULL;

// We don't need this malloc_count. You can remove it.
long malloc_count;


static inline size_t _align_up_bytes(size_t size){
	return (size + 7) & ~7;
}

static inline size_t _align_up_page(size_t size){
	return (size + 4095) & ~4095;
}

static block_t *allocate_heap(size_t size) {
	size_t require_page = size + BLOCK_SIZE);
	size_t chunk_size = _align_up_page(require_page + CHUNK_SIZE);

	chunk_t *tmp_chunk = vmalloc(NULL,chunk_size);

	if(tmp_chunk==NULL)  return NULL;

	tmp_chunk->size = chunk_size;
	tmp_chunk->next = my_chunk;
	my_chunk = tmp_chunk;

//	block_t *tmp_block =NULL;
	block_t *tmp_block = (block_t*)((char *)tmp_chunk + CHUNK_SIZE);
	tmp_block->size = chunk_size - CHUNK_SIZE;
	tmp_block->is_free=1;
	tmp_block->next=free_list;
	free_list=tmp_block;

	return tmp_block;
}

static block_t *find_block(size_t size){
	block_t *tmp_block = free_list;

	while (tmp_block->next!=NULL ){
		if (tmp_block->size >=size){
			return tmp_block;
		}
		tmp_block=tmp_block->next;
	}

	return NULL;
}

static block_t * split_chunk(block_t* cur_blcok,size_t size){
	block_t *res_block = (block_t*)((char *)cur_blcok);
	cur_blcok->size -=size;
	cur_blcok = (block_t*)((char *)cur_blcok-(char *)size);



}

void *mymalloc(size_t size){
	size_t require_size = _align_up_bytes(size);
	size_t total_require = require_size+BLOCK_SIZE;


	block_t *tmp_block  =find_block(total_require);

	if(tmp_block!=NULL){
		if(allocate_heap(total_require)==NULL){
			spin_unlock(&big_lock);
			return NULL;
		}
	}

	block_t *res_block = split_chunk(tmp_blcok,total_require);
	return res_block;
}

void myfree(void *ptr) {

}



/*
void allocate_heap(size_t require_size) {
	size_t require_page = _align_up_page(require_size + sizeof(chunk_t)+sizeof (block_t));

	//初始化my_chunk
	if (my_chunk == NULL) {
		my_chunk = vmalloc(NULL, require_page);
		my_chunk->size = require_page;
		my_chunk->next = NULL;
	}

	if (free_list == NULL) {
		chunk_t tmp_chunk = vmalloc(NULL,require_page);
		tmp_chunk->size = require_page;
		tmp_chunk.next = NULL;
		my_chunk->next = tmp_chunk;

		free_list->next=NULL;
		free_list->is_free=1;
		free_list->size=tmp_chunk.size;
	}

	block* tmp_block;
	tmp_block->next=NULL;
	tmp_block->is_free=1;
	tmp_block->size=my_chunk->size;
	free_list->next = tmp_block;

	//遍历chunk，如果该chunk大小满足size，就分配给第一个free_list

}
//	while (cur!=NULL){
//		if(cur->size < require_page) {
//			chunk_t tmp_chunk = vmalloc(NULL,require_page);
//			tmp_chunk->size = require_page;
//			cur->next = tmp_chunk;
//			break;
//		}
//		cur = cur->next;
//	}

//	chunk_t tmp_chunk = vmalloc(NULL,require_page);
//	tmp_chunk->size = require_page;
//	cur->next = tmp_chunk;
//
//	free_list->size = cur->size;
//	free_list->is_free = 0;
//	free_list->next=NULL;
//
//
//	return;
//}
*/



/*
void *mymalloc(size_t size) {
	size_t require_size = _align_up_bytes(size);

	if(free_list ==NULL){
		free_list =  allocate_freelist(require_size);
		if(free_list ==NULL){
			return NULL;
		}
	}
}
*/

/*
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
*/