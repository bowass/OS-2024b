/**
 * virtmem.c 
 * Written by Michael Ballantyne 
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define TLB_SIZE 16
#define PAGES 256
#define PAGE_MASK 255
#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255
#define MEMORY_SIZE (PAGES * PAGE_SIZE)
#define BUFFER_SIZE 10

struct tlbentry {
    unsigned char logical;
    unsigned char physical;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];

// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

int tlb_find_physical(const int page);

int main(int argc, const char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage ./virtmem backingstore input\n");
        exit(1);
    }

    const char *backing_filename = argv[1]; 
    int backing_fd = open(backing_filename, O_RDONLY);
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 

    const char *input_filename = argv[2];
    FILE *input_fp = fopen(input_filename, "r");

    // Fill page table entries with -1 for initially empty table.
    int i;
    for (i = 0; i < PAGES; i++) 
    {
        pagetable[i] = -1;
    }

    // Character buffer for reading lines of input file.
    char buffer[BUFFER_SIZE];

    // Data we need to keep track of to compute stats at end.
    int total_addresses = 0;
    int tlb_hits = 0;
    int page_faults = 0;
	int logical_address, logical_page, physical_address, physical_page, value;
    // Number of the next unallocated physical page in main memory
    unsigned char free_page = 0;
	
    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) 
    {
		total_addresses++;
		logical_address = atoi(buffer);
		logical_page = logical_address >> OFFSET_BITS;
		physical_page = tlb_find_physical(logical_page);
		if (physical_page != -1) {
			tlb_hits++;
		} else {
			if (pagetable[logical_page] != -1) {
				physical_page = pagetable[logical_page];
			} else { // page fault
				page_faults++;
				physical_page = free_page++;
				if (physical_page * PAGE_SIZE >= MEMORY_SIZE)
					fprintf(stderr, "error: insufficent memory");
				memcpy(main_memory + (physical_page * PAGE_SIZE), backing + (logical_page * PAGE_SIZE), PAGE_SIZE);
				pagetable[logical_page] = physical_page;
			}
			tlbindex++;
			tlb[tlbindex % TLB_SIZE].logical = logical_page;
			tlb[tlbindex % TLB_SIZE].physical = physical_page;
		}
		physical_address = physical_page * PAGE_SIZE + (logical_address & OFFSET_MASK);
        value = main_memory[physical_address];
        printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
    }

    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

    return 0;
}

int tlb_find_physical(const int page)
{
	for (int i = 0; i < TLB_SIZE; i++) {
		if (page == tlb[i].logical)
			return tlb[i].physical;
	}
	return -1;
}
