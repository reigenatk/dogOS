#ifndef SYSTEM_CALL_H
#define SYSTEM_CALL_H
#include "types.h"
#include "lib.h"
#include "filesystem.h"
#include "RTC.h"
#include "keyboard.h"
#include "task.h"
#include "paging.h"

void syscall_interrupt_handler(uint32_t syscall_no, uint32_t arg1, 
  uint32_t arg2, uint32_t arg3);

void test_syscall();

// make a bunch of global system calls for usage
// Prototypes appear below. Unless otherwise specified, 
// successful calls should return 0, and failed calls should return -1.
int32_t halt(uint8_t status);

int32_t execute(const uint8_t* command);

int32_t read(int32_t fd, void* buf, int32_t nbytes);


int32_t write(int32_t fd, const void* buf, int32_t nbytes);


int32_t open(const uint8_t* filename);

int32_t close(int32_t fd);

int32_t getargs(uint8_t* buf, int32_t nbytes);

int32_t vidmap(uint8_t** screen_start);

int32_t set_handler(int32_t signum, void* handler_address);

int32_t sigreturn(void);

// these are the actual system calls

#endif
