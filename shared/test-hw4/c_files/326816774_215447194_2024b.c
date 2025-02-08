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

#define TLB_SIZE 16
#define PAGES 256
#define PAGE_MASK 255

#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255

#define MEMORY_SIZE PAGES * PAGE_SIZE

// Max number of characters per line of input file to read.
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
// void* convertShortPhysicalAddressToVirtualMemory(short physical_address);
// void* convertShortPhysicalAddressToVirtualMemory(short physical_address){
//     return (void*)(char)(physical_address);
// };
// -----------------------------------------------------------------
// -------- YOUR CODE HERE: helper functions of your choice --------
// -----------------------------------------------------------------


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

    // Number of the next unallocated physical page in main memory
    unsigned char free_page = 0;

    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL)
    {
        total_addresses++;
        int logical_address = atoi(buffer);
        // unsigned char logical_address = atoi(buffer);//can take logical addressess only under the value 255 I think.
        // I need an unsigned char with hier capacity up to 2^16.

        //logical_address shouldent need more then 16 bits sinse there are only 2^16 virtual address.
        //logical_address &= 0xFFFF;
        int value = -10;//rundom num
        unsigned char pageNum = logical_address >> OFFSET_BITS;//lets assume logical_address is represented like a number in binary and not a char if there is a diffrence.
        unsigned char pageOffset = logical_address & 0xFF;
        int physical_address;//MEMORY_SIZE is 16 bits
        int in_tlb = 0;
        for(int i = 0; i < TLB_SIZE; i++){
            if(tlb[i].logical == pageNum){
                physical_address = (((tlb[i].physical) << OFFSET_BITS) | pageOffset);

                in_tlb = 1;
                tlb_hits++;
                break;
            }
        }
        if(!in_tlb){

            if (pagetable[pageNum] != -1){
                physical_address = ((pagetable[pageNum] << OFFSET_BITS) | pageOffset);//combine the frame address with the offset for the actual address.
                tlb[tlbindex % TLB_SIZE].logical = (pageNum);
                tlb[tlbindex % TLB_SIZE].physical = pagetable[pageNum];

                tlbindex++;

            }
            else {
                page_faults++;
                //the page is not in the page table page fault

                if(free_page*PAGE_SIZE >= MEMORY_SIZE){
                    exit(1);
                    //out of memory bound --> no more free memory to alocate
                }
                else{
                    int physical_Page_address = free_page*PAGE_SIZE;
                    memcpy(main_memory+(physical_Page_address) ,backing+pageNum*PAGE_SIZE,PAGE_SIZE);
                    physical_address = ((physical_Page_address) + pageOffset);
                    pagetable[pageNum] = (physical_Page_address >> OFFSET_BITS);
                    free_page++;
                    tlb[tlbindex % TLB_SIZE].logical = (pageNum);
                    tlb[tlbindex % TLB_SIZE].physical = (physical_Page_address >> OFFSET_BITS);
                    tlbindex++;

                }
            }
        }
        value = main_memory[physical_address];
        // void* ptr = main_memory + physical_address;
        // int value =1;
        // int value = *((signed char *)main_memory + physical_address);
        // int value = main_memory[]
        //int value = main_memory[physical_address];
        // signed char value = *(main_memory+physical_address);
        // -------------------------------------------------------------------------------------
        // -------- YOUR CODE HERE: translation of logical address to physical address  --------
        // -------------------------------------------------------------------------------------

        printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
    }

    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

    return 0;
}
