#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "filesystem.h"

#define PASS 1
#define FAIL 0

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
	}

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
	if (ret == -1) {
		return FAIL;
	}
	// printf("filename: %s filetype: %d inode: %d reserved: %s", 
	// t.file_name, t.file_type, t.inode_number, t.reserved);

	dentry_t t2;
	uint32_t ret2 = read_dentry_by_name(".", &t2);
	if (ret2 == -1) {
		return FAIL;
	}
	// printf("filename: %s filetype: %d inode: %d reserved: %s", 
	// t2.file_name, t2.file_type, t2.inode_number, t2.reserved);

	// check if the filenames are the same
	if (strncmp(&t.file_name, &t2.file_name, strlen(t.file_name) != 0)) {
		printf("3");
		return FAIL;
	}


	return PASS;
}

int read_data_test() {
	TEST_HEADER;
	uint8_t buffer[6224];
	uint32_t ret = read_data(40, 0, buffer, 6224);
	if (ret == -1) {
		return FAIL;
	}

	// this doesn't work, IDK why, keeps printing "S"
	// printf("%s\n", buffer);

	// int i;
	// for (i = 0; i < strlen(buffer); i++) {
	// 	putc(buffer[i]);
	// }

	// test going over the limit
	uint8_t buffer2[6224];
	uint32_t ret2 = read_data(40, 0, buffer2, 10000);
	if (ret2 != -1) {
		return FAIL;
	}

	return PASS;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	TEST_OUTPUT("idt_offset_test", idt_offset_test());
	TEST_OUTPUT("read_dentry_test", read_dentry_test());
	TEST_OUTPUT("read_data_test", read_data_test());

	// launch your tests here
}
