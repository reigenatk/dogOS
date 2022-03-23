#ifndef PAGING_H
#define PAGING_H

#define NUM_PAGE_ENTRIES 1024
#define FOURKB 4096
#define FOURMB 0x400000
#define FIRST_TEN_BITS_MASK 0xFFC00000
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

// remaps a virtual address to a specific physical address, by going into the page
// entry for that virtual address, and changing the physical address listed in the entry.
void virtual_to_physical_remap_directory(uint32_t virtual, uint32_t 
physical, uint32_t user_bit, uint32_t is_four_mb);

void virtual_to_physical_remap_table(uint32_t virtual, uint32_t physical, uint32_t user_bit);

// align page tables on 4KB boundaries
extern uint32_t page_directory[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));
extern uint32_t page_table[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));

#endif
