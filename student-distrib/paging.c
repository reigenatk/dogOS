#include "paging.h"


page_directory_entry page_directory[NUMENTRIES] __attribute__((aligned(FOURKB)));
page_table_entry page_table[NUMENTRIES] __attribute__((aligned(FOURKB)));

void setup_paging() {
  // 0 to 4MB should be 4KB pages, so 1024 of those
  // 4MB to 8MB is the kernel, in one 4MB page
  // 8MB to 4GB is all empty pages

  // each page table can handle 2^10 * 2^12 = 2^22 = 4MB of memory. Thus we need 2 page tables
  // one for the first 4MB and one for the second. We should set bit 7 of the second page directory entry to 1 
  // to indicate that it points to a 4MB page table
  
  uint32_t first_20_digits_mask = 0xFFFFF000;

  page_directory_entry first_entry;
  memset(&first_entry, 0, sizeof(page_directory_entry)); // 0x00000000
  first_entry.fourkb.present = 1;   
  first_entry.fourkb.read_write = 1;
  page_directory[0] = first_entry;
  page_directory[0].entry |= (((uint32_t) &page_table) & first_20_digits_mask); // top 20 bits of base address of page table

  
  // start paging at base address 0, for the very first page table

  uint32_t address = 0;
  int i;
  for (i = 0; i < NUMENTRIES; i++) {
    page_table_entry entry;
    memset(&entry, 0, sizeof(page_table_entry)); // 0x00000000
    entry.present = 1;
    entry.read_write = 1;

    entry.page_base_address = address & first_20_digits_mask;
    i += FOURKB;
  }

  // setup second page directory entry

  page_directory_entry second_entry;
  memset(&second_entry, 0, sizeof(page_directory_entry)); // 0x00000000
  second_entry.fourmb.present = 1;
  second_entry.fourmb.read_write = 1;
  second_entry.fourmb.page_size = 1;
  page_directory[1] = second_entry;

  uint32_t first_10_digits_mask = 0xFFC00000;
  // we want linear address 4MB to translate to 4MB physical. So since first 10 bits of page directory 4mb entry is 
  // the page base address (which we know we want as 0x00400000) then top 10 bits of this address is what we want
  page_directory[1].entry |= (0x00400000 & first_10_digits_mask); // top 20 bits of base address of page table
  
  // turn on paging by setting CR3 to the page directory's address
  // also set bit 32 of cr0 for page enable. And finally set bit 4 in cr4 for page size extension

  uint32_t address_of_page_directory = (uint32_t) &page_directory;
  asm volatile (
    "movl %0, %%cr3;"
    "movl %%cr0, %%eax;"
    "or 0x80000000, %%eax;"
    "movl %%eax, %%cr0;"
    "movl %%cr4, %%eax;"
    "or 0x00000010, %%eax;"
    "movl %%eax, %%cr4;"
    :
    : "r"(address_of_page_directory)
    : "%eax"
  );


}
