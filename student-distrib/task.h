#ifndef TASK_H
#define TASK_H

#include "filesystem.h"
#include "RTC.h"
#include "keyboard.h"
#define MAX_OPEN_FILES 8


/*
The final bit of per-task state that needs to be allocated is a kernel stack 
for each user-level program. Since
you only need to support two tasks, you may simply place the first task’s 
kernel stack at the bottom of the 4 MB kernel
page that you have already allocated. The second task’s stack can then go 
8 kB above it. This way, both tasks will have
8 kB kernel stacks to use when inside the kernel. Each process’s PCB 
should be stored at the top of its 8 kB stack, and
the stack should grow towards them. Since you’ve put both stacks inside the 
4 MB page, there is no need to “allocate”
memory for the process control block
*/
#define TASK_STACK_SIZE 0x2000 // (8kb per task for the stack)
#define KERNEL_MEM_BOTTOM 0x800000 // 8MB

/*
The program image itself is linked to execute at virtual address 0x08048000. The way to get this
working is to set up a single 4 MB page directory entry that maps virtual address 0x08000000 (128 MB) to the right
physical memory address (either 8 MB or 12 MB). Then, the program image must be copied to the correct offset
(0x00048000) within that page.

*/
#define PROGRAM_IMAGE_VIRTUAL_ADDRESS 0x08000000 // 128 MB
#define OFFSET_WITHIN_PAGE 0x00048000
/*
The file operations jump table associated with the correct file type. 
This jump table should contain entries
for open, read, write, and close to perform type-specific actions 
for each operation. open is used for
performing type-specific initialization. For example, if we just open’d 
the RTC, the jump table pointer in this
structure should store the RTC’s file operations table.
*/
typedef struct jump_table_fd {
  int32_t (*read)(int32_t, void*, int32_t);
  int32_t (*write)(int32_t, const void*, int32_t);
  int32_t (*open)(const uint8_t*);
  int32_t (*close)(int32_t);
} jump_table_fd;

// for flags bit, let's let the very first bit be whether or not the FD is in use.
// it will be set to 1 if in use, 0 otherwise.
#define FD_IN_USE_BIT 0x1

// lets also define read + write privs for certain file descriptors. For example, stdin
// is read only while stdout is write only
#define FD_READ_PERMS 0x2
#define FD_WRITE_PERMS 0x4

typedef struct file_descriptor {
  jump_table_fd jump_table;
  uint32_t inode;
  uint32_t file_position;
  uint32_t flags;

} file_descriptor;

// this is our pcb (Process Control Block) struct with data like all file descriptors,
// the name of the task, the arguments that were passed in (which is limited by 
// max buffer size), etc. This data goes at the start of the 8KB kernel stack for this task
typedef struct task {
  file_descriptor fds[MAX_OPEN_FILES];
  uint8_t name_of_task[32];
  uint32_t pid; // the process ID tells us all sorts of into about where the process is in memory
  uint32_t esp;
  uint8_t arguments[128];

} task;

/*
When processing the execute system call, your kernel
must create a virtual address space for the new process. This
will involve setting up a new Page Directory with entries
corresponding to the figure shown on the right.

In other words, each task has its own page directory!
*/

extern task cur_task;

void init_task();

uint32_t find_unused_fd();

/*
As in Linux, the tasks will share common mappings for kernel pages, in this case a single, global 4 MB page. Unlike
Linux, we will provide you with set physical addresses for the images of the two tasks, and will stipulate that they
require no more than 4 MB each, so you need only allocate a single page for each task’s user-level memory

TLDR: each task gets 4 MB page of memory.
*/

/*
To make physical memory management easy, you may
assume there is at least 16 MB of physical memory
on the system. Then, use the following (static) strategy: the first user-level 
program (the shell) should be
loaded at physical 8 MB, and the second user-level program, 
when it is executed by the shell, should be loaded at
physical 12 MB.
*/
uint32_t calculate_task_physical_address(uint32_t task_num);

uint32_t calculate_task_pcb_pointer(uint32_t task_num);


#endif