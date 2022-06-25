#ifndef PAGING_H
#define PAGING_H

#define NUM_PAGE_ENTRIES 1024
#define FOURKB 4096
#define FOURMB 0x400000
#define FIRST_TEN_BITS_MASK 0xFFC00000
#define NEXT_TEN_BITS 0x003FF000
#define FIRST_TWENTY_BITS 0xFFFFF000

// PDE/PTE flags
#define PRESENT_BIT 0x00000001
#define READ_WRITE_BIT 0x00000002
#define PAGE_SIZE_BIT 0x00000080
#define USER_BIT 0x00000004
#define GLOBAL_BIT 0x100

// flags for page entries
#define KERNEL_PAGE 0x2
#define RESERVE_PAGE 0x4

#define CR4_MASK 0x00000010
#define CR0_MASK 0x80000000
#define VIDEO_MEM_PAGE_TABLE_ENTRY 0xB8

#include "types.h"
#include "lib.h"



void setup_paging();

// /*
// map 0xB8000 to either 0xB8000 if the current visible terminal = the terminal that is running
// or map 0xB8000 to one of 0xB9000, 0xBA000, or 0xBB000 which is not the current displayed terminal
// */
// void map_video_mem(uint32_t terminal_no);

/*
Maps a 4MB page for process memory
pid = 1 : 0x8000000 -> 0x80000 (8MB)
pid = 2 : 0x8000000 -> 0xC0000 (12MB)
so on...
*/
void map_process_mem(uint32_t pid);

typedef struct fourkb_page_descriptor {
    uint32_t refcount; // how many virt addresses map to this page?
    uint32_t flags;
} fourkb_page_descriptor;

typedef struct fourmb_page_descriptor{
	uint32_t				flags;	///< private flags for physical memory administration
	int 					refcount;	///< use count or reference count
	struct fourkb_page_descriptor* 	pages; ///<  The pages
} fourmb_page_descriptor;

/**
 * @brief Equivalent to page_tab_add_entry in saenaios. Basically just adds a page table entry
 *  given virt, phys + flags
 * @param physical 
 * @param flags 
 * @return int32_t 
 */
int32_t map_virt_to_phys(uint32_t virtual, uint32_t physical, uint32_t flags);

int page_dir_add_4MB_entry(uint32_t virtual_addr, uint32_t real_addr, int flags);

/**
 * @brief If value is null, then it will populate phys_addr with phys mem to use
 *  otherwise it will increase reference count to that memory 
 *  So in other words, pass in an address if you know what physical addr you wanna use, otherwise just 
 * dont pass in anything.
 * @param phys_addr 
 * @return uint32_t 
 */
int32_t alloc_4kb_mem(uint32_t* phys_addr);

/**
 * @brief 4MB page version of above
 * 
 */
int32_t alloc_4mb_mem(uint32_t* phys_addr);

/**
 *
 *	exposed function to free a 4MB page
 *
 *	@param physical_addr: value of the physical address to free
 *
 *	@return 0 for success, negative value for error
 *
 *	@note actually decrease the use count of that memory
 */
int page_alloc_free_4MB(int physical_addr);

/**
 *
 *	exposed function to free a 4KB page
 *
 *	@param physical_addr: value of the physical address to free
 *
 *	@return 0 for success, negative value for error
 *
 *	@note actually decrease the use count of that memory
 */
int page_alloc_free_4KB(int physical_addr);

/**
 * @brief changes present bit on corresponding page directory to 0
 * 
 * @param virt 
 * @return int32_t 
 */
int32_t page_dir_delete_entry(uint32_t virt);

/**
 * @brief changes present bit on corresponding page table to 0
 * 
 * @param virt 
 * @return int32_t 
 */
int32_t page_tab_delete_entry(uint32_t virt);

// align page tables on 4KB boundaries
extern uint32_t page_directory[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));
extern uint32_t page_table[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));

// another page table available for use.
// the other page table is being used for first 4MB and is full.
// we don't want to overwrite that so we make a new one 
extern uint32_t userspace_page_table[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));
#endif
