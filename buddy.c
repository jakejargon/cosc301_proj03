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
		//return NULL if there is no available space in memory
		if ((freelist[(tempi/4)+1] == 0) && (freelist[tempi/4] < powerof_two)) {
			return NULL;
		}
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
		block_to_allocate[(size/4)+1] = size;
		}
	block_to_allocate[0] = size;
	block_to_allocate[1] = size; //we will change this
	}
	//change the offset of the previous free block to point to the new next free block
		//if the block we have just allocated was the beginning of the freelist
	if (tempi == 0) {
		freelist = &(freelist[(freelist[1])/4]);
	}
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

void free(void* memory_block) {
	
	printf("beginning to free...\n");
	int lead_block;
	int status;//use this variable to tell if buddy is left or right
	uint32_t *heapbegin = heap_begin;
	uint32_t *memblock = memory_block;
	int size;
	int lag_block;

	while(1) {
	
		//reinitialize
		lead_block = 0;
		status = 0;//use this variable to tell if buddy is left or right	
		size = memblock[0];	
		lag_block = 0;
	
		//check first block
		if (&heapbegin == &memblock) {}
		//lead_block has not encountered the memory block to be freed
		else {
			lead_block = size;
			status = 1;
			while (&heapbegin[lead_block/4] != &memblock[0]) {
				status += 1;
				lead_block+= size;
			}
		}
	
		//exiting this conditional block, I should know whether to look for a buddy to the left or right (see status)
	
		//sift through freelist to find the freeblock to the left of memblock
		lead_block = 0;
	
	
		lead_block = freelist[1];
		while (&freelist[lead_block] < memblock) {
			lead_block += freelist[(lead_block/4) +1];
			lag_block += freelist[(lag_block/4) +1];
		}
		//lag_block now has the index in freelist that is before memblock
		//if the buddy is to the right
		if ((status % 2 == 0) && (&freelist[(lead_block/4) -1] == &memblock[size/4])) {
			//change the previous freelist entry to point to the newly freed block
			freelist[(lag_block/4) +1] -= size;
			//change the header info on the newly freed block
			memblock[0] = size*2;
			if (freelist[(lead_block/4) + 1] == 0) {
				memblock[1] = 0;
			}
			else {
				memblock[1] = freelist[(lead_block/4) + 1] + size;
			}
			continue;
		}
		//buddy could be to the left
		else if ((status % 2 == 1) && (&freelist[(lag_block/4) + (size/4) + 1] == &memblock[(size/4)])) {
			freelist[lag_block/4] += size;
			memblock = &freelist[lag_block/4];
			continue;
		}	
		//there are no buddies
		else {
			//set offset for the previous freeblock
			freelist[(lag_block/4) +1] = (memblock - &freelist[lag_block/4])*sizeof(int);
			//create an offset on memblock
			memblock[1] = (&freelist[lead_block/4] - memblock)*sizeof(int);
			break;
		}
	}
}


void dump_memory_map(void) {
	
	printf("***dumping memory map***\n");
	int offset = (freelist - (uint32_t*)heap_begin) * sizeof(int);
	if (offset) {
		printf("block size: %d, offset 0, allocated\n", offset);
	}
	int free_index = 0;
	 
	while (offset < HEAPSIZE) {

		//we have reached the end of free memory
		if (freelist[(free_index/4) +1] == 0) {
			printf("block size: %d, offset %d, free\n", freelist[(free_index)/4], offset);
			offset += freelist[free_index/4];
			//check if there is another allocated block
			if (offset < HEAPSIZE) {
				printf("block size: %d, offset %d, allocated\n", (HEAPSIZE - offset), offset);
				break;
			}
		}
		
		//there is a free block ahead of the current free block
		else if (freelist[(free_index/4) +1] == freelist[free_index/4]) {
			printf("block size: %d, offset %d, free\n", freelist[free_index/4], offset);
			//add the size of the free block to get the beginning of the allocated block
			offset += freelist[free_index/4];
			free_index += freelist[free_index/4];
		}
		
		//there is an allocated block ahead of the current free block	
		else {
			printf("block size: %d, offset %d, free\n", freelist[free_index/4], offset);
			//add the size of the free block to get the beginning of the allocated block
			offset += freelist[free_index/4];
			printf("block size: %d, offset %d, allocated\n", (freelist[(free_index/4)+1] - freelist[free_index/4]), offset);
			//move the offset up again and change free_index to point to the next block
			offset += (freelist[(free_index/4)+1] - freelist[free_index/4]);
			free_index += freelist[(free_index/4)+1];
		}
	}
}

