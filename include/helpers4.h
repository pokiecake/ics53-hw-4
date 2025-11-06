#ifndef HELPERS4_H
#define HELPERS4_H
#include "icsmm.h"
// A header file for helpers.c
// Declare any additional functions in this file

#define BLOCK_MULTIPLE_SIZE 16
#define MAX_PAGES 6
#define PAGE_SIZE 4096
#define HEADER_SIZE 8
#define PROLOGUE_SIZE HEADER_SIZE
#define EPILOGUE_SIZE HEADER_SIZE
#define PRO_EPI_SIZE (PROLOGUE_SIZE + EPILOGUE_SIZE)

void initialize_prologue(ics_footer * prologue);

void initialize_epilogue(ics_header * epilogue);

void set_header(ics_header * header, uint64_t size, uint64_t is_allocated);

void set_footer(ics_footer * footer, uint64_t size, uint64_t requested_size, uint64_t is_allocated);

void set_free_header(ics_free_header * free_header, uint64_t size, ics_free_header * next, ics_free_header * prev);

char * allocate_block(uint64_t block_size, uint64_t requested_size, char * cur_addr);

void remove_block_from_list(ics_free_header * target, ics_free_header * new_block);

#endif
