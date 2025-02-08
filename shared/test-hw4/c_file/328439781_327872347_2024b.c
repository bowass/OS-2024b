#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 256
#define PAGE_MASK 255

#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255

#define MEMORY_SIZE PAGES * PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

typedef unsigned char address;

struct tlbentry {
	unsigned char logical;
	unsigned char physical;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];

// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char* backing;

//m func.check if a logical page is in the tlb:
int tlbCheck(address log_p)
{
	for (int i = 0; i < TLB_SIZE; i++)
	{
		if (tlb[i].logical == log_p)
			return tlb[i].physical;
	}
	return -1;
}

//adding a page to tlb(U use the tlbIndex to know exactly where)
int tlbAdd(address log_p, address phy_p) {
	tlb[tlbindex % TLB_SIZE].logical = log_p;
	tlb[tlbindex % TLB_SIZE].physical = phy_p;
	return 0;
}

// returning the physical address from the page table
int pageTableCheck(address log_p) {
	return pagetable[log_p];
}


int main(int argc, const char* argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Usage ./vitmem backingstore input\n");
		exit(1);
	}

	const char* backing_filename = argv[1];
	int backing_fd = open(backing_filename, O_RDONLY);
	backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

	const char* input_filename = argv[2];
	FILE* input_fp = fopen(input_filename, "r");

	// Fill the page table entries with -1 for initally empty table.
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
	//here are the new values:

	int physical_page = 0;
	int physical_address = 0;
	int offset = 0;
	int value = 0;
	int logical_page = 0;

	// Number of the next unallocated physical page in main memory
	unsigned char free_page = 0;

	while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL)
	{
		int logical_address = atoi(buffer);
		//int Physical_address = new int;
		//int offset = new int;
		//value = new int;

		total_addresses++;
		logical_page = (logical_address >> OFFSET_BITS) & PAGE_MASK;
		offset = logical_address & OFFSET_MASK;

		if (tlbCheck(logical_page) == -1) // then the page isn't in the tlb
		{
			if (pageTableCheck(logical_page) == -1) // then the page isn't in the page table yet
			{
				page_faults++;
				physical_page = free_page;
				free_page++;
				pagetable[logical_page] = physical_page;
				for (int i = 0; i < PAGE_SIZE; i++) {
					main_memory[physical_page * PAGE_SIZE + i] = backing[logical_page * PAGE_SIZE + i];
				}
			}
			else // the page IS in the page table
			{
				physical_page = pageTableCheck(logical_page);
			}
			tlbindex++;
			tlbAdd(logical_page, physical_page);

		}
		else // the page is in the tlb
		{
			tlb_hits++;
			physical_page = tlbCheck(logical_page);
		}
		value = main_memory[physical_page * PAGE_SIZE + offset];
		physical_address = physical_page * PAGE_SIZE + offset;
		//printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
		printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
		//printf("Virtual address: %d \n", total_addresses);
	}

	printf("Number of Translated Addresses = %d\n", total_addresses);
	printf("Page Faults = %d\n", page_faults);
	printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
	printf("TLB Hits = %d\n", tlb_hits);
	printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

	return 0;
}
