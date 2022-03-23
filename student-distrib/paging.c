#include "paging.h"
#include "lib.h"

uint32_t page_directory[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));

uint32_t page_table[NUM_PAGE_ENTRIES] __attribute__((aligned(FOURKB)));
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
  // 8MB to 4GB is all empty pages

  // each page table can handle 2^10 * 2^12 = 2^22 = 4MB of memory. Thus we need 2 page tables
  // one for the first 4MB and one for the second. We should set bit 7 of the second page directory entry to 1 
  // to indicate that it points to a 4MB page table
  
  /* present bit 0
    Is the page present?
  */

  /* read-WRITE bit 1
    Specifies the read-write privileges for a page or group of pages (in the case of
    a page-directory entry that points to a page table). When this flag is clear, the
    page is read only; when the flag is set, the page can be read and written into.
  */

  /* User/Supervisor Bit 2
  Specifies the user-supervisor privileges for a page or group of pages (in the
  case of a page-directory entry that points to a page table). When this flag is
  clear, the page is assigned the supervisor privilege level; when the flag is set,
  the page is assigned the user privilege level. 
  */

 // 3 = read write privs and present page, supervisor privs

  //initialize page directory and page table entries to default values
  int i;
  for (i = 0; i < FOURKB; i++) {
    page_table[i] = ((i*FOURKB) & FIRST_TWENTY_BITS) | READ_WRITE_BIT; 
    page_directory[i] = READ_WRITE_BIT;

  }

  page_directory[0] = ((uint32_t) page_table) | (PRESENT_BIT | READ_WRITE_BIT);
  page_directory[1] = (FOURMB & FIRST_TEN_BITS_MASK) | (PRESENT_BIT | READ_WRITE_BIT) | PAGE_SIZE_BIT; 
  // video mem
  page_table[0xB8] |= (PRESENT_BIT | READ_WRITE_BIT); 

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


void virtual_to_physical_remap_directory(uint32_t virtual, uint32_t physical, uint32_t user_bit, uint32_t is_four_mb) {
  uint32_t page_idx = virtual & FIRST_TEN_BITS_MASK; // which index into the page table are we at 
  if (is_four_mb == 0) {
    page_directory[page_idx] = (physical & FIRST_TWENTY_BITS) | PRESENT_BIT | READ_WRITE_BIT;
  }
  else {
    page_directory[page_idx] = (physical & FIRST_TEN_BITS_MASK) | PRESENT_BIT | READ_WRITE_BIT;
  }
  
  if (user_bit == 1) {
    page_directory[page_idx] |= USER_BIT;
  }


}

void virtual_to_physical_remap_table(uint32_t virtual, uint32_t physical, uint32_t user_bit) {
  // first 10 bits are offset into page directory, next 10 is entry into table,
}
