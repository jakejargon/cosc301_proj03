#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

// function declarations
void *malloc(size_t);
void free(void *);
void dump_memory_map(void);


//const int HEAPSIZE = (1*1024*1024); // 1 MB
const int HEAPSIZE = (1024); //changed the heap to 1KB to make it easier for me
const int MINIMUM_ALLOC = sizeof(int) * 2;

// global file-scope variables for keeping track
// of the beginning of the heap.
void *heap_begin = NULL; 
//initialize freelist
uint32_t* freelist = NULL;


void *malloc(size_t request_size) {

    // if heap_begin is NULL, then this must be the first
    // time that malloc has been called.  ask for a new
    // heap segment from the OS using mmap and initialize
    // the heap begin pointer.
    
    if (!heap_begin) {
        heap_begin = mmap(NULL, HEAPSIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
        atexit(dump_memory_map);
        //my code here vv
        freelist = heap_begin;
        freelist[0] = HEAPSIZE;
		freelist[1] = 0;
    }	
   
	//make request_size bigger for the header
	request_size = request_size + (sizeof(int)*2);
	//find nearest power of two
	int powerof_two = 8;
	while (request_size > powerof_two) {
		powerof_two*=2;
	}
	printf("request size: %d\n", powerof_two);
	int tempi = 0;
	int prev_pointer = 0;
	//go through the freelist using the offset and check if powerof_two will fit
	while (freelist[tempi/4] < powerof_two) {
		prev_pointer = tempi;
		tempi += freelist[(tempi/4)+1];//modify tempi to point to the next freeblock by following the offset, hence the +1
	}
	
	uint32_t* block_to_allocate = (freelist + (tempi/4));
	
	int first = 1;
	int size = *(block_to_allocate + 0);
	//break the free block into smaller pieces, don't modify leftmost block yet
	//while loop breaks when size of the allocated block is the closest power of two greater than or equal to the size we need
	while (size != powerof_two) {
		size = size/2;
		//write the header info to the new rightmost block
		*(block_to_allocate + (size/4)) = size;
		//this has the rightmost block point to the next free space
		if (first) {
			if (!*(block_to_allocate + 1)) {
				*(block_to_allocate + (size/4) + 1) = 0;
			}
			else {
			*(block_to_allocate + (size/4)+1) = block_to_allocate[1] - size;//take the original offset of the free block and adjust the offset to match the halved size
			}
			first = 0;
		}
		else {
		//if we have split this further, have the offset point to the block to the right of it
		block_to_allocate[((size/4)+1)] = size;
		}
	block_to_allocate[0] = size;
	block_to_allocate[1] = size; //we will change this
	}
	//change the offset of the previous free block to point to the new next free block
		//if the block we have just allocated was the beginning of the freelist
	if (tempi == 0) {
		freelist = &(freelist[(freelist[1])/4]);
	}
	 /*&& block_to_allocate[1]) {
		freelist = &(freelist[(freelist[1])/4]);
	}
	else if (tempi == 0 && !block_to_allocate[1]) {
		freelist = &(freelist[size/4]);
	}*/
	else {
		freelist[(prev_pointer/4) + 1] = (freelist[(prev_pointer/4) + 1]) + size;
	}
	//modify the leftmost block
	block_to_allocate[1] = 0; // write header offset, this will be 0 until free is called.
	
	printf("freelist info (size, offset): (%d, %d)\n", freelist[0], freelist[1]);
	printf("allocated info (size, offset): (%d, %d)\n", block_to_allocate[0], block_to_allocate[1]);
	printf("fin\n"); 
	
	return (void *)( block_to_allocate + 2);
}

void free(void *memory_block) {

}

void dump_memory_map(void) {
	
	int initial_chunk = (freelist - heapbegin) * sizeof(int);
	if (initial_chunk) {
		printf("block size: %d, offset 0, allocated\n", initial_chunk);
	}
	int pointer = 0;
	while (pointer != HEAPSIZE) {
		
		
}

