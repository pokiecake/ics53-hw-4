#include "helpers4.h"
#include "debug.h"
#include "icsmm.h"

/* Helper function definitions go here */
void initialize_prologue(ics_footer * prologue) {
	prologue->block_size = 1; //0 size, 1 allocated
	prologue->fid = FOOTER_MAGIC;
	prologue->requested_size = 0;
}

void initialize_epilogue(ics_header * epilogue) {
	epilogue->block_size = 1; //0 size, 1 allocated
	epilogue->hid = HEADER_MAGIC;
}

void set_header(ics_header * header, uint64_t size, uint64_t is_allocated) {
	header->block_size = size + is_allocated; //pack the allocated bit into the size bits
	header->hid = HEADER_MAGIC;
}

void set_footer(ics_footer * footer, uint64_t size, uint64_t requested_size, uint64_t is_allocated) {
	footer->block_size = size + is_allocated;//pack the allocated bit into the size bits
	if (is_allocated) //only change requested_size if the block is allocated
		footer->requested_size = requested_size;
	footer->fid = FOOTER_MAGIC;
}

void set_free_header(ics_free_header * free_header, uint64_t size, ics_free_header * next, ics_free_header * prev) {
	set_header(&free_header->header, size, 0);
	free_header->next = next;
	free_header->prev = prev;
}

void remove_block_from_target_list(ics_free_header * target_block, ics_bucket * bucket_list) {
	ics_free_header * prev = target_block->prev, * next = target_block->next;
	if(prev != NULL) {
		prev->next = next;
	} else {
		bucket_list->freelist_head = next;
	}
	if (next != NULL) {
		next->prev = prev;
	}
}

void add_block_to_target_list(ics_free_header * target_block, ics_bucket * bucket_list) {
	ics_free_header * next = bucket_list->freelist_head;
	target_block->next = next;
	if (next != NULL) {
		next->prev = target_block;
	}
	bucket_list->freelist_head = target_block;
}

//void move_splinter_block(ics_free_header * target_block, int old_bucket_indx) 
void move_splinter_block(ics_free_header * target_block) {
	ics_bucket * new_bucket_list = NULL;
	// * old_bucket_list = seg_buckets[old_bucket_indx];
	int i;
	for (int i = 0; i < BUCKET_COUNT; i++) {
		if (seg_buckets[i].max_size >= target_block->header.block_size) {
			new_bucket_list = &seg_buckets[i];
		}
		if (seg_buckets[i].freelist_head == target_block) {
			seg_buckets[i].freelist_head = target_block->next;
		}
	}
	add_block_to_target_list(target_block, new_bucket_list);
	// ics_free_header cur_header * = new_bucket_list->freelist_head;
	// cur_header->prev = target_block;
	// target_block->next = cur_header;
	// new_bucket_list->freelist_head = target_block;
	/* 
	Kind of messy honestly, does a lot of weird things. Finds the relevant bucket list, also removes the block if its in the head, then calls the add function. Extra overhead if the element is at the front of the list. But it helps work with the coalesce function, since I didn't add a way to get the prev and next elements. :P
	*/
}

ics_bucket * find_best_fit_list(uint64_t block_size) {
	int i;
	for (i = 0; i < BUCKET_COUNT; i++) {
		//find first available bucket that fits the block size
		if (seg_buckets[i].freelist_head != NULL && seg_buckets[i].max_size >= block_size) {
			return &seg_buckets[i];
		}
	}
	return NULL;
}

ics_free_header * find_available_block(uint64_t block_size, ics_bucket ** bucket) {
	int i;
	// for (i = 0; i < BUCKET_COUNT; i++) {
	// 	if (seg_buckets[i]->max_size >= block_size) {
	// 		*bucket_index = i; 
	// 		return seg_buckets[i]->freelist_head;
	// 	}
	// }
	//ics_bucket * best_list = find_best_fit_list(block_size);
	for (i = 0; i < BUCKET_COUNT; i++) {
		ics_bucket * best_list = seg_buckets + i;
		ics_free_header * free_block = NULL;
		if (best_list->freelist_head == NULL || best_list->max_size < block_size) {
			continue;
		}
		//while (best_list != NULL) {
		*bucket = best_list;
		free_block = best_list->freelist_head;
		//find the first free block in the list that can hold the requested block size, or NULL if there is none
		while (free_block != NULL && free_block->header.block_size < block_size) {
			free_block = free_block->next;
		}
		if (free_block != NULL) {
			return free_block;
		}
		//}
	}
	return NULL;
}

//cur_addr points to the free header
void * allocate_block(uint64_t block_size, uint64_t requested_size, char * cur_addr, ics_bucket * bucket) {
	ics_free_header * free_header = (ics_free_header *)(cur_addr);
	void * alloc_block = NULL;
	if (free_header->header.block_size >= block_size) {
		//If there is fragmentation, allocate all the space
		if (free_header->header.block_size - block_size < 32) {
			//no splitting
			// remove block from prev and next pointers 
			remove_block_from_list(free_header, NULL, bucket);
			ics_header * allocated_header = (ics_header *)free_header;
			ics_footer * alloc_footer = (ics_footer *)((char *)(free_header) + allocated_header->block_size - HEADER_SIZE);
			allocated_header->block_size += 1; // set allocated bit to 1
			set_footer(alloc_footer, allocated_header->block_size, requested_size, 1);
			// alloc_footer-block_size = allocated_header->block_size;
			// alloc_footer->requested_size = requested_size;
			// alloc_footer->fid = FOOTER_MAGIC;
			
			alloc_block = (void *)((char *)allocated_header + HEADER_SIZE); //to point the block to the payload
		}
		//if there is no fragmentation, allocate just enough and split the free block
		else {
			//create a new free block, updating its size accordingly
			uint64_t new_free_block_size = free_header->header.block_size - block_size;
			//cur_addr points to payload (next ptr), so must subtract header to get the true location of the future header.
			ics_free_header * new_free_header = (ics_free_header *)(cur_addr + block_size - HEADER_SIZE);
			set_free_header(new_free_header, new_free_block_size, NULL, NULL);
			// set prev (or head) and next lists to point to new free block
			remove_block_from_list(free_header, new_free_header, bucket);

			ics_header * alloc_header = &free_header->header;
			ics_footer * alloc_footer = (ics_footer *)((char *)(alloc_header) + block_size - 8);

			//set the headers for the allocated block
			set_header(alloc_header, block_size, 1);
			//footer will be 8 bytes away from the end of the block
			set_footer(alloc_footer, block_size, requested_size, 1);
			
			alloc_block = (void *)((char *)(alloc_header) + HEADER_SIZE); //to point the block to the payload
		}
	}

	return alloc_block;
}

void remove_block_from_list(ics_free_header * target, ics_free_header * new_block, ics_bucket * old_bucket) {
	ics_free_header * prev = target->prev, * next = target->next;
	remove_block_from_target_list(target, old_bucket);
	// if (prev != NULL) {
	// 	prev->next = (new_block == NULL ? next : new_block);
	// 	next->prev = (new_block == NULL ? prev : new_block);
	// 	// set pointers for new_block properly
	// 	if (new_block != NULL) {
	// 		//!! first check if the new block fits within the segregated list
	// 		new_block->next = next;
	// 		new_block->prev = prev;
	// 	}
	// }	
	// else {
	// 	seg_buckets[old_bucket_indx] = next;
	// 	//find which list has the target as its head, then move the head to the next block;
	// 	// int i;
	// 	// for (i = 0; i < BUCKET_COUNT; i++) {
	// 	// 	ics_free_header * list_head = (seg_buckets + i)->freelist_head;
	// 	// 	if (list_head == target) {
	// 	// 		(seg_buckets + i)->freelist_head = target->next;
	// 	// 		break;
	// 	// 	}
	// 	// }
	// }
	if (new_block != NULL) {
		//check if splinter best fits in this bucket
		if (find_best_fit_list(new_block->header.block_size) == old_bucket) {
			if (prev != NULL) {
				prev->next = new_block;
			} else {
				old_bucket->freelist_head = new_block;
			}
			if (next != NULL) {
				next->prev = new_block;
			}
			new_block->next = next;
		} else {
			move_splinter_block(new_block);
		}
	}
}

void * calc_next_block_header(void * bp) { //pointing to header of current block
	ics_header * header = (ics_header *)bp;
	return bp + (header->block_size & ~0x7);
}

void * calc_prev_block_footer(void * bp) {
	return bp - HEADER_SIZE; //points to footer of previous block
}

void * calc_header_from_footer(void * bp) { //bp is a footer
	ics_footer * footer = (ics_footer *)bp;
	return bp - (footer->block_size & ~0x7) + HEADER_SIZE; //footer is HEADER_SIZE away from the end of the block
}

void * calc_footer_from_header(void * bp) { //bp is a header
	ics_header * header = (ics_header *)bp;
	return bp + (header->block_size & ~0x7);
}

//returns pointer to start of new pointer
ics_free_header * coalesce(void * bp) { //pointer to free block header
	ics_header * next_block = calc_next_block_header(bp);
	ics_footer * prev_block = calc_prev_block_footer(bp);
	ics_free_header * coalesced_header = (ics_free_header *)bp;
	uint64_t middle_block_size = coalesced_header->header.block_size;
	int next_block_alloc = next_block->block_size & 0x1, prev_block_alloc = prev_block->block_size & 0x1;
	if (next_block_alloc == 1 && prev_block_alloc == 1) {
		return coalesced_header;
	} else if (next_block_alloc == 0 && prev_block_alloc == 1) {
		//get the footer of the next block.
		//update the sizes for the current block header and next block's footer
		ics_footer * new_footer = calc_footer_from_header((void *)next_block);
		coalesced_header->header.block_size += new_footer->block_size;
		new_footer->block_size = coalesced_header->header.block_size;
		
		//move relevant next and prev pointers!!
		//make into a function?
		ics_free_header * old_free_header = (ics_free_header *)next_block;
		ics_free_header * prev = old_free_header->prev, * next = old_free_header->next;
		if (prev != NULL)
			prev->next = next;
		if (next != NULL)
			next->prev = prev;
	} else if (next_block_alloc == 1 && prev_block_alloc == 0) {
		ics_footer * new_footer = calc_footer_from_header(coalesced_header);
		//header of the bp pointer is now irrelevant
		coalesced_header = calc_header_from_footer((void *)prev_block);
		coalesced_header->header.block_size += new_footer->block_size;
		new_footer->block_size = coalesced_header->header.block_size;
		//move relevant next and prev pointers!!
		//make into a function?
		ics_free_header * prev = coalesced_header->prev, * next = coalesced_header->next;
		if (prev != NULL)
			prev->next = next;
		if (next != NULL)
			next->prev = prev;
	 } else {
		uint64_t free_block_size = coalesced_header->header.block_size; //get block size before losing the middle block header
		ics_footer * new_footer = calc_footer_from_header((void *)next_block);
		coalesced_header = calc_header_from_footer((void *)prev_block);
		coalesced_header->header.block_size += new_footer->block_size + free_block_size;
		new_footer->block_size = coalesced_header->header.block_size;
		//move relevant next and prev pointers!!
		//make into a function?
		ics_free_header * prev = coalesced_header->prev, * next = coalesced_header->next;
		if (prev != NULL)
			prev->next = next;
		if (next != NULL)
			next->prev = prev;
	}
	

	return coalesced_header;
}

void * get_new_page(void ** new_brk) {
	void * old_brk = ics_inc_brk(); //get the old brk pointer (points after the old epilogue)
	*new_brk = ics_get_brk(); //update brk_ptr in the icsmm.c file
	ics_footer * old_footer = old_brk - EPILOGUE_SIZE - HEADER_SIZE; //old footer is located before the epilogue

	//establish the epilogue and new footer
	ics_header * epilogue = (ics_header *)(*new_brk - EPILOGUE_SIZE);
	initialize_epilogue(epilogue);
	ics_footer * new_footer = (ics_footer *)(*new_brk - EPILOGUE_SIZE - HEADER_SIZE);
	ics_free_header * new_header = (ics_free_header *)old_brk;
	set_free_header(new_header, PAGE_SIZE - PRO_EPI_SIZE, NULL, NULL);
	//coalesce to old fragment.
	new_header = coalesce(old_brk);
	set_footer(new_footer, new_header->header.block_size, 0, 0);

	// !!move coalesced chunk to relevant list
	//add free header to list
	move_splinter_block(new_header);

	return old_brk;
}