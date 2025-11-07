#include "icsmm.h"
#include "debug.h"
#include "helpers4.h"
#include <stdio.h>
#include <stdlib.h>

//moved to helpers as macros
// const int BLOCK_MULTIPLE_SIZE = 16;
// const int PAGE_SIZE = 4096;
// const int HEADER_SIZE = 8;
// const int PROLOGUE_SIZE = HEADER_SIZE;
// const int EPILOGUE_SIZE = HEADER_SIZE;
// const int PRO_EPI_SIZE = PROLOGUE_SIZE + EPILOGUE_SIZE;
// const int MAX_PAGES = 6;
void * mem_ptr = NULL;
void * brk_ptr = NULL;

#define DEBUG_PRINTING


void *ics_malloc(size_t size) { 
	//first time: allocate space (huge chunk of memory)
	//-install prologue and epilogue (8 byters)
	//-install header and footer
	if (mem_ptr == NULL) { //very first call to malloc
		// !!check for too many pages
		mem_ptr = ics_inc_brk(); 
		brk_ptr = ics_get_brk();
		ics_footer * prologue = (ics_footer *)mem_ptr; 
		ics_free_header * start_header = (ics_free_header *)(mem_ptr + PROLOGUE_SIZE);
		ics_footer * footer = (ics_footer *)(brk_ptr - EPILOGUE_SIZE - HEADER_SIZE); 
		ics_header * epilogue = (ics_header *)(brk_ptr - EPILOGUE_SIZE);
		
		initialize_prologue(prologue);
		initialize_epilogue(epilogue);
		//set the header of the free block with the size = current page size - size of prologue and epilogue
		//set it as the start of the linked list (in the buckets??)
		set_free_header(start_header, PAGE_SIZE - PRO_EPI_SIZE, NULL, NULL);
		set_footer(footer, PAGE_SIZE - PRO_EPI_SIZE, 0, 0); // requested size is arbitary for a free block?
		
		seg_buckets[BUCKET_COUNT - 1].freelist_head = start_header; //make the head of the biggest list the header of the whole block
		//DEBUG
		#ifdef DEBUG_PRINTING
			fflush(stdout);
			printf("!!DEBUG PRINTING, DELETE IN FINAL VERSION!!\n");
			fflush(stdout);
			ics_freelist_print(4);
			//ics_header_print((void *)start_header);
			ics_header_print(epilogue);
			printf("!!END DEBUG PRINTING!!\n");
			getchar();
			fflush(stdout);
			
		#endif
	}
	
	
	//calculate minimum block size
	size_t remainder = size % BLOCK_MULTIPLE_SIZE;
	// if size is a multiple of 16, payload is the size. Else, move on to the next multiple of 16;
	size_t payload_size = (remainder == 0 ? size : (size / BLOCK_MULTIPLE_SIZE + 1) * BLOCK_MULTIPLE_SIZE);
	size_t block_size = payload_size + 16;
	//find bucket to hold data
	ics_bucket * bucket;
	ics_free_header * free_block = find_available_block(block_size, &bucket);

	while (free_block == NULL) {
		// !!check for more than 6 pages
		get_new_page(&brk_ptr);
		free_block = find_available_block(block_size, &bucket);
	}
	//-if no splinter, split free block into allocated space and free chunck, update headers
	//--put free chunk into a fitting list LIFO
	//-if splinter, take the whole chunk and updat headers
	void * ret_block_addr = allocate_block(block_size, size, (char *)free_block, bucket);
	
	
    return ret_block_addr;//return address of payload
}


int ics_free(void *ptr) { 
    return -99999;
}
