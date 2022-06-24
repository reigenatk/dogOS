#ifndef SYSTEM_CALL_H
#define SYSTEM_CALL_H
#include "types.h"
#include "libc/signal.h"
#include "libc/sys/types.h"
#include "ece391sysnum.h"

void syscall_interrupt_handler(uint32_t syscall_no, uint32_t arg1, 
  uint32_t arg2, uint32_t arg3);

void test_syscall();

// special function to change from ring 0 to ring 3 (in .S file)
int32_t change_task(uint32_t entry);

// ++++++++++++++++++++++++++++++++++++ 
// the below system calls are ALL from the ASM macro btw, and simply serve as an entry point into the real system call!
// ++++++++++++++++++++++++++++++++++++

// make a bunch of global system calls for usage
// Prototypes appear below. Unless otherwise specified, 
// successful calls should return 0, and failed calls should return -1.

// 391 syscalls
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

// signals
int32_t sigaction(int signum, int sigaction_ptr, int oldsigaction_ptr);

int32_t kill(pid_t pid, int sig);

int32_t sigsuspend(const sigset_t *mask);

int32_t sigprocmask(int how, int setp, int oldsetp);

// tasks


// jump table
typedef int32_t (*syscall_handler)(int, int, int);
extern syscall_handler jump_table[NUM_SYSCALLS];

void register_all_syscalls();

#endif
