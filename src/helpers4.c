#include "helpers4.h"
#include "debug.h"

/* Helper function definitions go here */
void initialize_prologue(ics_footer * prologue) {
	prologue->block_size = 1; //0 size, 1 allocated
	prologue->fid = FOOTER_MAGIC;
	prologue->requested_size = 0
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
	footer->requested_size = requested_size;
	footer->fid = FOOTER_MAGIC;
}

void set_free_header(ics_free_header * free_header, uint64_t size, uint64_t is_allocated, ics_free_header * next, ics_free_header * prev) {
	set_header(free_header->header, size ,is_allocated);
	free_header->next = next;
	free_header->prev = prev;
}

char * allocate_block(uint64_t block_size, uint64_t requested_size, char * cur_addr) {
	ics_free_header * free_header = cur_addr - 8;
	if (free_header->header->size >= block_size) {
		//if there is no fragmentation, allocate just enough and split the free block
		uint64_t new_free_block_size = free_header->header->size - block_size;
		ics_free_header * new_block_next = free_header->next;
		ics_free_header * new_block_prev = free_header->prev;
		
		set_header(header, block_size, 1);
		//footer will be 8 bytes away from the end of the block
		set_footer(header + block_size - 8);

		set_free_header(header + block_size, new_free_block_size, 0, new_block_next, new_block_prev);
		// !!set linked list head to the new block if needed
		// !!set prev and next lists to point to new address
	}
}