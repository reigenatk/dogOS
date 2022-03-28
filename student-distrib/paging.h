#ifndef PAGING_H
#define PAGING_H

#define NUM_PAGE_ENTRIES 1024
#define FOURKB 4096
#define FOURMB 0x400000
#define FIRST_TEN_BITS_MASK 0xFFC00000
#define NEXT_TEN_BITS 0x003FF000
#define FIRST_TWENTY_BITS 0xFFFFF000
#define PRESENT_BIT 0x00000001
#define READ_WRITE_BIT 0x00000002
#define PAGE_SIZE_BIT 0x00000080
#define USER_BIT 0x00000004 
#define CR4_MASK 0x00000010
#define CR0_MASK 0x80000000

#include "types.h"
#include "lib.h"



void setup_paging();

// very similar to setup_paging, just initializes the page directory so it has 
// one entry to the process page table, and initializes the process page table
// so it has 1024 entries that map to 
// setup_paging_for_process(uint32_t pd_addr, uint32_t pt_addr);



// maps a virtual to physical by tracing the address translation that the virtual
// address would take, and creating corresponding page table + page directory entries 
// to get to that physical address.
void virtual_to_physical_remap_usertable(uint32_t virtual, uint32_t physical, uint32_t user_bit);

// align page tables on 4KB boundaries
extern uint32_t page_directory[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));
extern uint32_t page_table[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));

// another page table available for use.
// the other page table is being used for first 4MB and is full.
// we don't want to overwrite that so we make a new one 
extern uint32_t userspace_page_table[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));
#endif
