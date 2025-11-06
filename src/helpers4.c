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

void * allocate_block(uint64_t block_size, uint64_t requested_size, char * cur_addr) {
	ics_free_header * free_header = (ics_free_header *)(cur_addr - 8);
	void * alloc_block;
	if (free_header->header.block_size >= block_size) {
		//If there is fragmentation, allocate all the space
		if (free_header->header.block_size - block_size < 32) {
			//no splitting
			// !!remove block from prev and next pointers 
			remove_block_from_list(free_header, NULL);
			ics_header * allocated_header = (ics_header *)free_header;
			ics_footer * alloc_footer = (ics_footer *)((char *)(free_header) + allocated_header->block_size);
			allocated_header->block_size += 1; // set allocated bit to 1
			set_footer(alloc_footer, allocated_header->block_size, requested_size, 1);
			// alloc_footer-block_size = allocated_header->block_size;
			// alloc_footer->requested_size = requested_size;
			// alloc_footer->fid = FOOTER_MAGIC;
			
			alloc_block = (void *)(allocated_header + HEADER_SIZE); //to point the block to the payload
		}
		//if there is no fragmentation, allocate just enough and split the free block
		else {
			//create a new free block, updating its size accordingly
			uint64_t new_free_block_size = free_header->header.block_size - block_size;
			//cur_addr points to payload (next ptr), so must subtract header to get the true location of the future header.
			ics_free_header * new_free_header = (ics_free_header *)(cur_addr + block_size - HEADER_SIZE);
			set_free_header(new_free_header, new_free_block_size, NULL, NULL);
			// set prev (or head) and next lists to point to new free block
			remove_block_from_list(free_header, new_free_header);

			ics_header * alloc_header = &free_header->header;
			ics_footer * alloc_footer = (ics_footer *)(alloc_header + block_size - 8);

			//set the headers for the allocated block
			set_header(alloc_header, block_size, 1);
			//footer will be 8 bytes away from the end of the block
			set_footer(alloc_footer, block_size, requested_size, 1);
			
			alloc_block = (void *)(alloc_header + HEADER_SIZE); //to point the block to the payload
		}
	}

	return alloc_block;
}
void remove_block_from_list(ics_free_header * target, ics_free_header * new_block) {
	ics_free_header * prev = target->prev, * next = target->next;
	if (prev != NULL) {
		prev->next = (new_block == NULL ? next : new_block);
		next->prev = (new_block == NULL ? prev : new_block);
		// set pointers for new_block properly
		if (new_block != NULL) {
			//!! first check if the new block fits within the segregated list
			new_block->next = next;
			new_block->prev = prev;
		}
	}	
	else {
		//find which list has the target as its head, then move the head to the next block;
		int i;
		for (i = 0; i < BUCKET_COUNT; i++) {
			ics_free_header * list_head = (seg_buckets + i)->freelist_head;
			if (list_head == target) {
				(seg_buckets + i)->freelist_head = target->next;
				break;
			}
		}
	}
}
