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
char * mem_ptr = NULL;
char * brk_ptr = NULL;

void *ics_malloc(size_t size) { 
	if (mem_ptr == NULL) { //very first call to malloc
		// !!check for too many pages
		mem_ptr = ics_inc_brk(); 
		brk_ptr = ics_get_brk();
		ics_footer * prologue = mem_ptr; 
		ics_free_header * start_header = mem_ptr + PROLOGUE_SIZE;
		ics_footer * footer = brk_ptr - EPILOGUE_SIZE - HEADER_SIZE; 
		ics_header * epilogue = brk_ptr - EPILOGUE_SIZE;
		
		initialize_prologue(prologue);
		initialize_epilogue(epilogue);
		//set the header of the free block with the size = current page size - size of prologue and epilogue
		//set it as the start of the linked list (in the buckets??)
		set_free_header(start_header, PAGE_SIZE - PRO_EPI_SIZE, 0, NULL, NULL);
		set_footer(footer, PAGE_SIZE - PRO_EPI_SIZE, 0, 0); // requested size is arbitary for a free block?
	}
	
	//first time: allocate space (huge chunk of memory)
	//-install prologue and epilogue (8 byters)
	//-install header and footer
	
	//calculate minimum block size
	size_t remainder = size % BLOCK_MULTIPLE_SIZE;
	// if size is a multiple of 16, payload is the size. Else, move on to the next multiple of 16;
	size_t payload_size = (remainder * BLOCK_MULTIPLE_SIZE == size ? size : (remainder + 1) * BLOCK_MULTIPLE_SIZE;
	size_t block_size = payload_size + PRO_EPI_SIZE;
	//find bucket to hold data
	//-if no splinter, split free block into allocated space and free chunck, update headers
	//--put free chunk into a fitting list LIFO
	//-if splinter, take the whole chunk and updat headers
	
	
    return NULL;//return address of payload
}


int ics_free(void *ptr) { 
    return -99999;
}
