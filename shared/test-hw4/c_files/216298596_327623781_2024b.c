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

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

typedef int i32;
typedef short i16;
typedef unsigned char u8;
typedef signed char i8;

typedef struct {
    u8 logical;
    u8 physical;
} tlbentry;

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
tlbentry tlb[TLB_SIZE];

// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
i32 tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
i32 pagetable[PAGES];

i8 main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
const i8 *backing;

static i16 tlb_query(const u8 logical_page) {
    for (const tlbentry* e = tlb; e != tlb + TLB_SIZE; ++e)
        if (e->logical == logical_page) 
            return e->physical;
    return -1;
}
int main(const int argc, const char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage ./virtmem backingstore input\n");
        exit(1);
    }

    const char *backing_filename = argv[1]; 
    const i32 backing_fd = open(backing_filename, O_RDONLY);
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 

    const char *input_filename = argv[2];
    FILE *input_fp = fopen(input_filename, "r");

    // Fill page table entries with -1 for initially empty table.
    memset(pagetable, -1, PAGES * sizeof(i32));
    // Character buffer for reading lines of input file.
    char buffer[BUFFER_SIZE];

    // Data we need to keep track of to compute stats at end.
    i32 total_addresses = 0;
    i32 tlb_hits = 0;
    i32 page_faults = 0;

    // Number of the next unallocated physical page in main memory
    u8 free_page = 0;
    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
        const i32 logical_address = atoi(buffer);
        ++total_addresses;
        const u8 logical_page = logical_address >> OFFSET_BITS;

        const i16 result = tlb_query(logical_page);
        
        u8 physical_page;
        if (result != -1) { //tlb hit
            ++tlb_hits;
            physical_page = result;
        } else {
            if (pagetable[logical_page] != -1) {
                physical_page = pagetable[logical_page];
            } else { //page fault
                ++page_faults;
                physical_page = free_page++;
                memcpy(main_memory + (physical_page << OFFSET_BITS), backing + (logical_page << OFFSET_BITS), PAGE_SIZE);
                pagetable[logical_page] = physical_page;
            }
            tlbindex = (tlbindex + 1) % TLB_SIZE;
            tlb[tlbindex] = (tlbentry){logical_page, physical_page};
        }
        
        const i32 offset = logical_address & OFFSET_MASK;
        const i32 physical_address = (physical_page << OFFSET_BITS) | offset;
        const i8 value = main_memory[physical_address];

        printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
    }

    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

    return 0;
}
