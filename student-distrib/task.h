#ifndef TASK_H
#define TASK_H

#include "types.h"

// defined by MP3
#define MAX_OPEN_FILES 8
#define MAX_TASKS 6

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
#define FD_IN_USE 0x1

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

// forward declare so we can use it inside its own definition



typedef struct task {
  file_descriptor fds[MAX_OPEN_FILES];
  uint8_t name_of_task[32];
  uint32_t pid; // the process ID tells us all sorts of into about where the process is in memory
  
  // current ebp + esp
  uint32_t esp;
  uint32_t ebp;
  uint8_t arguments[128];

  // store process id of the parent task which called this task.
  // if nothing called this task, set this to its own process id
  uint32_t parent_process_task_id;

  // for when we call halt on this process (i.e end it), 
  // so we can return nicely to kernelspace
  // these are the values of esp, ebp from the sys_execute call that spawned 
  // this task
  uint32_t return_esp;
  uint32_t return_ebp;
  uint32_t return_eip;


  // uint32_t* page_table_address;
  // // page tables and directories are of type uint32_t, so we need pointers to that
  // uint32_t* page_directory_address;
} task;

/*
When processing the execute system call, your kernel
must create a virtual address space for the new process. This
will involve setting up a new Page Directory with entries
corresponding to the figure shown on the right.

In other words, each task has its own page directory!
*/


/*
Further, you must support up to six processes
in total. For example, each terminal running shell running 
another program. 
For the other extreme, have 2 terminals
running 1 shell and have 1 terminal running 4 programs 
(a program on top of shell, on top of shell, etc.).

Mark bit as 1 if process ID is taken, 0 if not taken
We will use IDs 1 - 6, hence the +1
*/
extern uint32_t running_process_ids[MAX_TASKS+1];

task *get_task();
task *get_task_in_running_terminal();

int32_t get_new_process_id();

uint32_t total_programs_running();

// Given a PID, this creates a new task struct for this process and initializes stdin, stdout file
// descriptors for it 
task* init_task(uint32_t pid);

int32_t find_unused_fd();

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
uint32_t calculate_task_physical_address(uint32_t pid);

uint32_t calculate_task_pcb_pointer(uint32_t pid);



#endif