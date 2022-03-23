#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "filesystem.h"
#include "interrupt_handlers.h"

#include "paging.h"

#define PASS 1
#define FAIL 0

int rtc_test_counter = 0;

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_offset_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 20; ++i){
		if (i == 15) {
			// fifteenth interrupt handler doesn't need to be set
			continue;
		}
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}

		// the other information
		if (idt[i].seg_selector != KERNEL_CS || idt[i].reserved4 != 0x00 || idt[i].reserved3 != 0
    || idt[i].reserved2 != 1 || idt[i].reserved1 != 1 || idt[i].size != 1 || idt[i].reserved0 != 0
    || idt[i].dpl != 0 || idt[i].present != 1) {
			result = FAIL;
		}

	}

	// test if the addresses are writing in properly, here we'll use the keyboard interrupt
	i = 0x21;
	unsigned int ptr = (unsigned int*) &keyboard_handler_wrapper;
	uint32_t first_16_bits = 0xFFFF0000;
	uint32_t last_16_bits = 0x0000FFFF;
	if ((idt[i].offset_31_16 != ((first_16_bits & ptr) >> 16)) || (idt[i].offset_15_00 != (last_16_bits & ptr))) 
	{
		result = FAIL;
	}

	return result;
}

/* RTC test
 * 
 * Checks to see if we are receiving RTC interrupts
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: RTC interrupt functionality
 * Files: RTC.c
 */
int rtc_test() {
	TEST_HEADER;
	int result = PASS;
	// check if the RTC value is not zero (we have set rate to 2 per sec)
	// theoretically a few seconds will have passed since the tests run last, but there's a 
	// chance that this fails
	if (rtc_test_counter == 0) {
		result = FAIL;
	}
	printf("# of RTC interrupts received: %d\n", rtc_test_counter);
	return result;
}

// int scancodes_test() {
// 	TEST_HEADER;
// 	int result = PASS;
// 	return result;
// }

int paging_test() {
	TEST_HEADER;
	int result = PASS;

	// check alignment
	if ((uint32_t) &page_directory % FOURKB != 0 || (uint32_t) &page_table % FOURKB != 0) {
		result = FAIL;
	}
	uint32_t address_of_page_table = &page_table;
	// check that the page directory's first entry points to the first 20 bits of the page table
	if (page_directory[0] & FIRST_TWENTY_BITS != address_of_page_table & FIRST_TWENTY_BITS) {

		result = FAIL;
	}

	
	// size checks
	if (sizeof(page_table[0]) != sizeof(uint32_t) || sizeof(page_directory[0]) != sizeof(uint32_t)) {

		result = FAIL;
	}

	// num of entries checks
	if (sizeof(page_table) / sizeof(page_table[0]) != NUM_PAGE_ENTRIES || sizeof(page_directory) / sizeof(page_directory[0]) != NUM_PAGE_ENTRIES) {
		result = FAIL;
	}

	int i;


	// check that page table is properly filled with addresses starting from 0 to 4096KB or 4MB
  uint32_t address = 0;

  for (i = 0; i < NUM_PAGE_ENTRIES; i++) {
    uint32_t entry = page_table[i];
    if ((entry & READ_WRITE_BIT != READ_WRITE_BIT)) {

			result = FAIL;
		}
		
		if (entry & FIRST_TWENTY_BITS != address & FIRST_TWENTY_BITS) {

			result = FAIL;
		}
		address += FOURKB;
  }

	// 4mb page for kernel code
	uint32_t second_entry = page_directory[1];

	if ((second_entry & PRESENT_BIT != PRESENT_BIT) || (second_entry & READ_WRITE_BIT != READ_WRITE_BIT)
	|| (second_entry & PAGE_SIZE_BIT != PAGE_SIZE_BIT)) {
		result = FAIL;
	}
	// if ((second_entry & FIRST_TEN_BITS_MASK) != (FOURMB & FIRST_TEN_BITS_MASK)) {
	// 	result = FAIL;
	// }

	// check that video memory was allocated
	if (page_directory[0xB8] & PRESENT_BIT != PRESENT_BIT) {
		result = FAIL;
	}

	// we have 8MB of memory, AKA 0x800000
	// let's try dereferencing memory out of this zone
	// uint32_t bad_addr = 0x800001;
	// uint32_t stuff = *((uint32_t*) bad_addr);

	// this generates page fault exception actually because the page is not present
	// uint32_t bad_addr2 = 0x42242;
	// uint32_t stuff2 = *((uint32_t*) bad_addr2);
	// printf("%d", stuff2);

	// this would be a good one (somewhere in kernel memory):
	// uint32_t good_addr = 0x500000;
	// uint32_t stuff3 = *((uint32_t*) good_addr);
	// printf("%d", stuff3);

	return result;
}

/* Checkpoint 2 tests */

// for personal reference 
// Entry 0: ., inode 0, filetype 1
// Entry 1: sigtest, inode 40, filetype 2. Datablocks 15,29, length 6224 bytes

int read_dentry_test() {
	TEST_HEADER;
	dentry_t t;
	uint32_t ret = read_dentry_by_index(1, &t);
	int result = PASS;
	if (ret == -1) {
		result = FAIL;
	}
	// printf("filename: %s filetype: %d inode: %d reserved: %s", 
	// t.file_name, t.file_type, t.inode_number, t.reserved);

	dentry_t t2;
	uint32_t ret2 = read_dentry_by_name("sigtest", &t2);
	if (ret2 == -1) {
		result = FAIL;
	}
	printf("filename: %s filetype: %d inode: %d reserved: %s\n", 
	t2.file_name, t2.file_type, t2.inode_number, t2.reserved);

	// check if the filenames are the same
	if (strncmp(&t.file_name, &t2.file_name, strlen(t.file_name) != 0)) {
		printf("3");
		result = FAIL;
	}

	// print all filenames + sizes as suggested in test
	int i;
	for (i = 0; i < 17; i++) {
		uint32_t ret = read_dentry_by_index(i, &t);
		printf("name: %s, size: %d bytes, inode: %d\n", t.file_name, get_file_size(t.inode_number), t.inode_number);
	}

	

	return result;
}

int read_data_test() {
	TEST_HEADER;
	int result = PASS;
	uint8_t buffer[6224];

	// inode 40 is the sigtest ELF file
	uint32_t ret = read_data(40, 0, buffer, 6224);
	uint8_t correct_string[] = "2017-04-24, 16:44:13";
	if (ret == -1 || strncmp(buffer, correct_string, strlen(correct_string))) {

		result = FAIL;
	}

	// this doesn't work, IDK why, keeps printing "S"
	// printf("%s\n", buffer);

	// print out random bytes of some executable as suggested
	int i;
	for (i = 0; i < strlen(buffer); i++) {
		putc(buffer[i]);
	}

	// test going over the limit with offset
	uint8_t buffer2[6224];
	uint32_t ret2 = read_data(40, 10000, buffer2, 10);
	if (ret2 != -1) {
		
		result = FAIL;
		
	}

	// test reading data given a filename
	uint8_t buffer3[1000];
	uint8_t name[5] = "hello";
	read_data_by_filename(name, buffer3, 100);
	for (i = 0; i < strlen(buffer3); i++) {
		putc(buffer3[i]);
	}
	

	return result;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	// TEST_OUTPUT("idt_offset_test", idt_offset_test());

	// TEST_OUTPUT("paging_test", paging_test());
	// // run this last
	// TEST_OUTPUT("receive_rtc_interrupt", rtc_test());

	// Checkpoint 2 Tests
	TEST_OUTPUT("read_dentry_test", read_dentry_test());
	TEST_OUTPUT("read_data_test", read_data_test());

	// launch your tests here
}
