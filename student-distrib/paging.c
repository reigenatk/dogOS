#include "paging.h"
#include "lib.h"
#include "task.h"
#include "errno.h"

// -------------------------------- BEGIN IMPORTANT STUFF --------------------------------------------------------------- //

uint32_t page_directory[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));

uint32_t page_table[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));

uint32_t userspace_page_table[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));

// another page table to use, allocatable mem (for example for signal_user.S)
uint32_t allocatable_mem[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));

#define GET_PAGEDIR_IDX(x) (x/FOURMB) // first 10 bits
#define GET_PAGETAB_IDX(x) ((x / FOURKB) & (0x3FF)) // next 10 bits

// a description of the free memory to allocate. We are doing 4 * 1024 4KB chunks
// which is 4*4MB which is 16MB of memory, starting at physical addr 0x10000000
fourkb_page_descriptor allocatable_mem_table[4][1024];

// so the range is from really 64 -> 100 *4MB, so 256 to 400MB. First 4 pages are for 4kB, other 32 are for 4MB.
#define ALLOCATABLE_MEM_END_RANGE 100

// a description of the free fourmb pages to allocate (here I did a bit more than 512 since 2GB is 4MB * 512 = 2048MB)
fourmb_page_descriptor fourmb_mem_table[ALLOCATABLE_MEM_END_RANGE];

// physical. we will start allocing mem here @ 2^28 or 256MB
// first we have four 4MB pages which each have 1024 4KB pages to alloc out, which is described by the "allocatable_mem_table" above
// then immediately after we have 100-68 = 32 4MB pages to alloc out which is described by fourmb_mem_table
#define ALLOCATABLE_MEM_START 0x10000000

uint32_t mem_allocate_idx = 0;

#define ALLOCATABLE_MEM_FOURMB_START 68 // 64-67 taken up by allocatable mem. The way we get 64btw is 4mb = 2^22 and allocatable starts at 2^28 so its 2^6 difference

uint32_t fourmb_mem_allocate_idx = ALLOCATABLE_MEM_FOURMB_START;

// -------------------------------- END IMPORTANT STUFF -------------------------------------------------------------- //

/**
 * @brief Turns on paging for the intel processor
 * 
 */
void turn_on_paging() {
    // I tried doing this in one big asm volatile but it wouldn't work for some reason
  // turn on paging by setting CR3 to the page directory's address
	asm volatile(
    "movl %0, %%cr3" 
    :
    : "r"(page_directory) 
  );
  uint32_t val;
  // set bit 4 in cr4 for page size extension
	asm volatile(
    "movl %%cr4, %0"
		: "=r"(val)
  );
  val |= CR4_MASK;
  //stores back into cr4 to actually enable 4MB paging
	asm volatile(
    "movl %0, %%cr4"
		:
		: "r"(val)
  );
	// also set bit 32 of cr0 for page enable. 
	asm volatile(
    "movl %%cr0, %0"
		: "=r"(val)
  );
	val |= CR0_MASK;
	asm volatile(
    "movl %0, %%cr0"
		:
		: "r"(val)
  );
}

int32_t add_page_table(uint32_t virtual, uint32_t page_table_addr, uint32_t flags) {
  uint32_t page_dir_idx = GET_PAGEDIR_IDX(virtual);
  if (page_directory[page_dir_idx] & PRESENT_BIT || page_directory[page_dir_idx] & PAGE_SIZE_BIT) {
    return -EINVAL;
  }

  // if the page table addr is not aligned to 4KB boundary
	if (((int)page_table_addr) % FOURKB){
		return -EINVAL;
	}

  page_directory[page_dir_idx] = (page_table_addr & FIRST_TWENTY_BITS) | flags;
}

/* void setup_paging 
 * DESCRPITION: This function set up the paging  0 to 4MB should be 4KB pages, so 1024 of those 4MB to 8MB is the kernel, in one 4MB page
*                8MB to 4GB is all empty pages, each page table can handle 2^10 * 2^12 = 2^22 = 4MB of memory. Thus we need 2 page tables
*                  one for the first 4MB and one for the second. We should set bit 7 of the second page directory entry to 1 
*                  To indicate that it points to a 4MB page table
*  inputs:          NONE
*  outputs:         none
*   sideaffects paging is set up
*/
void setup_paging() {
  // 0 to 4MB should be 4KB pages, so 1024 of those
  // 4MB to 8MB is the kernel, in one 4MB page
  // 8MB to 4GB is all empty pages, each process builds up a 4MB chunk starting at 8MB.
  // we will set 16MB of memory free to be allocated, starting at 2GB (so from 2GB to 2GB+16MB)

  // start by setting all the data structures to zero
  // memset(page_table, 0, sizeof(page_table));
  // memset(page_directory, 0, sizeof(page_directory));
  
  

  //initialize page directory and page table entries to default values
  int i;
  for (i = 0; i < FOURKB; i++) {
    page_table[i] = ((i*FOURKB) & FIRST_TWENTY_BITS) | READ_WRITE_BIT; 
    page_directory[i] = READ_WRITE_BIT;
  }
  
  // video mem
  // In this layout everything in the first 4MB, that isn’t the page for video memory, 
  // should be marked not present. It's B8 because it's the B8'th page that will translate
  // to the page with base address 0xB8000. 
  // Base adress was already assigned in the for loop above, as was read/write.
  // We just want to mark it present.

  page_table[VIDEO_MEM_PAGE_TABLE_ENTRY] |= (PRESENT_BIT | USER_BIT);

  // these three entries are for each of the three terminals, each a 4KB offset higher than real video mem
  page_table[VIDEO_MEM_PAGE_TABLE_ENTRY + 1] |= (PRESENT_BIT | USER_BIT);
  page_table[VIDEO_MEM_PAGE_TABLE_ENTRY + 2] |= (PRESENT_BIT | USER_BIT);
  page_table[VIDEO_MEM_PAGE_TABLE_ENTRY + 3] |= (PRESENT_BIT | USER_BIT);

  fourmb_mem_table[0].refcount = 1;
  fourmb_mem_table[1].refcount = 1;

  // the four 4MB pages, totalling 16MB starting at 0x10000000. Mark these as counted
  fourmb_mem_table[64].refcount = 1;
  fourmb_mem_table[64].pages = allocatable_mem_table[0]; 
  fourmb_mem_table[65].refcount = 1;
  fourmb_mem_table[65].pages = allocatable_mem_table[1]; 
  fourmb_mem_table[66].refcount = 1;
  fourmb_mem_table[66].pages = allocatable_mem_table[2]; 
  fourmb_mem_table[67].refcount = 1;
  fourmb_mem_table[67].pages = allocatable_mem_table[3]; 

  // assign some custom flags as well
  fourmb_mem_table[0].flags |= KERNEL_PAGE;
  fourmb_mem_table[1].flags |= KERNEL_PAGE;
  fourmb_mem_table[2].flags |= KERNEL_PAGE;

  fourmb_mem_table[64].refcount |= RESERVE_PAGE;
  fourmb_mem_table[65].refcount |= RESERVE_PAGE;
  fourmb_mem_table[66].refcount |= RESERVE_PAGE;
  fourmb_mem_table[67].refcount |= RESERVE_PAGE;

  // 0MB - 4MB is just page_table
  add_page_table(0, page_table, PRESENT_BIT | READ_WRITE_BIT);

  // 4MB - 8MB is kernel code
  page_directory[1] = (FOURMB & FIRST_TEN_BITS_MASK);
  page_directory[1] |= (PRESENT_BIT | READ_WRITE_BIT) | PAGE_SIZE_BIT; 

  // 8MB - 12MB is kernel stacks?
  page_directory[2] = (FOURMB & FIRST_TEN_BITS_MASK);
  page_directory[2] |= (PRESENT_BIT | READ_WRITE_BIT) | PAGE_SIZE_BIT;

  // for signals asm page?
  add_page_table(ALLOCATABLE_MEM_START, allocatable_mem, PRESENT_BIT | READ_WRITE_BIT | USER_BIT);
  turn_on_paging();
}



int32_t increase_4kb_refcount(uint32_t phys_addr) {
  // check to see if allocated first
  if (allocatable_mem_table[GET_PAGEDIR_IDX(phys_addr)][GET_PAGETAB_IDX(phys_addr)].refcount <= 0) {
    return -EINVAL;
  }
  allocatable_mem_table[GET_PAGEDIR_IDX(phys_addr)][GET_PAGETAB_IDX(phys_addr)].refcount++;
  return 0;
}

int32_t increase_4mb_refcount(uint32_t phys_addr) {
  // check to see if allocated first
  if (fourmb_mem_table[GET_PAGEDIR_IDX(phys_addr)].refcount <= 0) {
    return -EINVAL;
  }
  fourmb_mem_table[GET_PAGEDIR_IDX(phys_addr)].refcount++;
  return 0;
}

int page_alloc_free_4MB(int physical_addr){
	// never free a kernel page 
	if (physical_addr <= FOURMB){
		return -EINVAL;
	}
	// check if the physical page is really in use
	if (fourmb_mem_table[GET_PAGEDIR_INDEX(physical_addr)].refcount<=0){
		return -EINVAL;
	}
	// then decrease the use count
	fourmb_mem_table[GET_PAGEDIR_INDEX(physical_addr)].refcount--;
	return 0;
}

int page_alloc_free_4KB(int physical_addr){
	// never free a kernel page or a manyoushu page
	if (physical_addr <= ALLOCATABLE_MEM_START || physical_addr > ALLOCATABLE_MEM_END_RANGE*FOURMB){
		return -EINVAL;
	}
	// check if the physical page is really in use
	if (allocatable_mem_table[GET_PAGEDIR_INDEX(physical_addr) - 64][GET_PAGETAB_IDX(physical_addr)].refcount<=0){
		return -EINVAL;
	}
	// then decrease the use count
	allocatable_mem_table[GET_PAGEDIR_INDEX(physical_addr) - 64][GET_PAGETAB_IDX(physical_addr)].refcount--;
	return 0;
}

/**
 * @brief 4MB version of alloc
 * 
 * @return uint32_t 
 */
int32_t alloc_empty_4mb_mem() {
  int count = 0;
  while (fourmb_mem_table[fourmb_mem_allocate_idx].refcount > 0) {
    count++;
    fourmb_mem_allocate_idx++;
    if (fourmb_mem_allocate_idx == ALLOCATABLE_MEM_END_RANGE) {
      fourmb_mem_allocate_idx = ALLOCATABLE_MEM_FOURMB_START; // wrap around and check lower addresses again
    }
    if (count == ALLOCATABLE_MEM_END_RANGE - ALLOCATABLE_MEM_START) {
      // if we are still here, means all the physical pages were allocated. Shucks!
      return -ENOMEM;
    }
  }
  // otherwise, we found some phys mem with no references. Increment ref and return it
  fourmb_mem_table[fourmb_mem_allocate_idx].refcount++;
  return fourmb_mem_allocate_idx * (FOURMB);
}


int32_t alloc_4mb_mem(uint32_t* phys_addr) {
  if (!phys_addr) {
    return -EINVAL;
  }
  if (*phys_addr != 0) {
    increase_4mb_refcount(*phys_addr);
  }
  else {
    uint32_t ret = alloc_empty_4mb_mem();
    if (ret > 0) {
      *phys_addr = ret;
      return 0;
    }
    else {
      return ret;
    }
  }
}

/**
 * @brief When you "alloc" memory we have to increase its refcount in the memtables
 *  that way when we try to alloc more memory, the memtable will say that this phys mem is used
 * so what this function does is just cycles thru all 16MB of allocatable mem (a limit we set)
 * and then checks if any of the physical pages are refcount = 0, that is, free. If so 
 * it returns the address of that physical page.
 * @return uint32_t 
 */
int32_t alloc_empty_4kb_mem() {
  int count = 0;
  while (allocatable_mem_table[mem_allocate_idx / 1024][mem_allocate_idx % 1024].refcount > 0) {
    count++;
    mem_allocate_idx++;
    if (mem_allocate_idx == 4096) {
      mem_allocate_idx = 0; // wrap around and check lower addresses again
    }
    if (count == 4096) {
      // if we are still here, means all the physical pages were allocated. Shucks!
      return -ENOMEM;
    }
  }
  // otherwise, we found some phys mem with no references. Increment ref and return it
  allocatable_mem_table[mem_allocate_idx / 1024][mem_allocate_idx % 1024].refcount++;
  return ALLOCATABLE_MEM_START + mem_allocate_idx * (FOURKB);
}

int32_t alloc_4kb_mem(uint32_t* phys_addr) {
  if (!phys_addr) {
    return -EINVAL;
  }
  if (*phys_addr != 0) {
    increase_4kb_refcount(*phys_addr);
  }
  else {
    uint32_t ret = alloc_empty_4kb_mem();
    if (ret > 0) {
      *phys_addr = ret;
      return 0;
    }
    else {
      return ret;
    }
  }
}

// 0x08000000 = 128 MB, and since we take first 10 bits its 0000 | 1000 | 00, aka 32th entry
// in the page directory table. We need this to be a 4MB page, and also a user page (cuz its a user process' memory)
void map_process_mem(uint32_t pid) {
  // remap the virtual memory at 0x8000000 to the right 4MB page for this task
  uint32_t start_physical_address = calculate_task_physical_address(pid);
  page_directory[32] = start_physical_address | PRESENT_BIT | READ_WRITE_BIT | PAGE_SIZE_BIT | USER_BIT;

  // must flush to inform the TLB of new changes to paging structures. Because 
  // otherwise the processor will use the (wrong) page cache
  flush_tlb();
}

int page_dir_add_4MB_entry(uint32_t virtual_addr, uint32_t real_addr, int flags){
	// check inconsistent flag
	if (!(flags & PAGE_SIZE_BIT)){
		return -EINVAL;
	}
	// auto fit to nearest 4MB
	int page_dir_index;
	page_dir_index = GET_PAGEDIR_INDEX(virtual_addr);

	// if this dir entry already exists
	if (page_directory[page_dir_index] & (PRESENT_BIT)){
		return -EEXIST;
	}

	virtual_addr = page_dir_index * FOURMB;
	real_addr = (real_addr / FOURMB) * FOURMB;

	flags |= PRESENT_BIT; // sanity force present flags

	// if want to map to a kernel address, hmmmmmm deny it because that would be bad and break everything
	if (fourmb_mem_table[GET_MEM_MAP_INDEX(real_addr)].flags & (KERNEL_PAGE)){
		return -EACCES;
	}

	// if want to map to a physical memory that hasn't been allocated
	if (fourmb_mem_table[GET_MEM_MAP_INDEX(real_addr)].refcount <= 0){
		return -EINVAL;
	}

	// add the entry in page directory
	page_directory[page_dir_index] = (real_addr) | flags;

	return 0;
}

/*
Page/Page Table base address, bits 12 through 32
(Page-table entries for 4-KByte pages.) Specifies the physical address of the
first byte of a 4-KByte page. The bits in this field are interpreted as the 
20 most significant bits of the physical address, which forces pages to be aligned on
4-KByte boundaries.
*/
int32_t map_virt_to_phys(uint32_t virtual, uint32_t physical, uint32_t flags) {
  // first 10 bits are offset into page directory, next 10 is offset into table, final is offset into page
  uint32_t offset_into_pd = GET_PAGEDIR_IDX(virtual);
  uint32_t offset_into_pt = GET_PAGETAB_IDX(virtual);

  // check if page directory entry present
  if (!(page_directory[offset_into_pd] & PRESENT_BIT)) {
    return -EINVAL;
  }

  if (flags & PAGE_SIZE_BIT) {
    // its a 4MB page
    
  }

  uint32_t* page_table = (uint32_t*) (page_directory[offset_into_pd] & FIRST_TWENTY_BITS);


  // check if this virtual addr has already been allocated to a page yet. If so then don't change it since
  // it already exists
  if (!page_table || page_table[offset_into_pt] & PRESENT_BIT == 1) {
    return -EEXIST;
  } 

  // check if the 4MB page its a part of and the 4kb page itself has been allocated, that is, refcount != 0
  // also check if the page inside of that is marked as already having been allocated.
  // key idea, if not allocated then dont map the memory. We have to keep track of every memory we give away so allocate first.
  if (fourmb_mem_table[GET_PAGEDIR_IDX(physical)].refcount <= 0 || fourmb_mem_table[GET_PAGEDIR_IDX(physical)].pages[GET_PAGETAB_IDX(physical)].refcount <= 0) {
    return -EACCES;
  }


  // ok its fine, let's do the mapping
  page_table[offset_into_pt] = (physical & FIRST_TWENTY_BITS) | flags;
  return 0;
}

int32_t page_dir_delete_entry(uint32_t virt) {
  uint32_t offset_into_pd = GET_PAGEDIR_IDX(virt);
  if (page_directory[offset_into_pd] & PRESENT_BIT) {
    page_directory[offset_into_pd] &= ~PRESENT_BIT;
    return 0;
  }
  else {
    // not present
    return -EINVAL;
  }
}

int32_t page_tab_delete_entry(uint32_t virt) {
  uint32_t offset_into_pd = GET_PAGEDIR_IDX(virt);
  uint32_t offset_into_pt = GET_PAGETAB_IDX(virt);
  if (page_directory[offset_into_pd] & PRESENT_BIT) {
    uint32_t* page_table = (uint32_t*) (page_directory[offset_into_pd] & FIRST_TWENTY_BITS);
    // check if page table entry even exists or is already deleted
    if (!page_table || page_table[offset_into_pt] & PRESENT_BIT == 0) {
      return -EEXIST;
    }
    else {
      page_table[offset_into_pt] &= ~PRESENT_BIT;
    }
  }
  else {
    // not present
    return -EINVAL;
  }
}