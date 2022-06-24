#ifndef TASK_H
#define TASK_H

#include "types.h"
#include "libc/signal.h"
#include "interrupt_handlers.h"

// defined by MP3
#define MAX_OPEN_FILES 8

// because there are max 256 spots for kernel stacks (4MB page / 16kb stack per task,
// though we could change size of kernel stack if we wanted more tasks)
#define MAX_TASKS 256


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

// tss.esp0 in scheduler and systemcalls.c use this
#define TSS_ESP_CALC(pid) KERNEL_MEM_BOTTOM - ((pid) * TASK_STACK_SIZE) - 4

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

// task states (only one should be set at a time) https://elixir.bootlin.com/linux/latest/source/include/linux/sched.h#L84
#define TASK_ST_NA			0	///< Process PID is not in use
#define TASK_ST_RUNNING		1	///< Process is actively running on processor
#define TASK_ST_SLEEP		2	///< Process is sleeping (also called Interruptable)
#define TASK_ST_ZOMBIE		3	///< Process is awaiting parent `wait()`
#define TASK_ST_DEAD		4	///< Process is dead

#define SIG_MAX 32

// this is our pcb (Process Control Block) struct with data like all file descriptors,
// the name of the task, the arguments that were passed in (which is limited by 
// max buffer size), etc. This data goes at the start of the 8KB kernel stack for this task
typedef struct task_t {
  uint8_t status; ///< Current status of this task
  uint8_t tty; ///< Attached tty number
  file_descriptor fds[MAX_OPEN_FILES];
  uint8_t name_of_task[32];
  uint32_t pid; // the process ID tells us all sorts of into about where the process is in memory
  uint32_t parent_pid; ///< parent process id

  
  struct s_regs regs;

  // signal stuff
  struct sigaction sigacts[32]; ///< Signal handlers
	sigset_t pending_signals;	///< Pending signals
	sigset_t signal_mask; ///< Deferred signals
	uint32_t exit_status; ///< Status to report on `wait`

  // current ebp + esp
  uint32_t esp;
  uint32_t ebp;
  uint32_t k_esp;
  uint8_t arguments[128];

  // store ptr to parent task. IF its the first shell it will point 
  // to the dummy one we made in the scheduler
  struct task *parent_task;

  // for when we call halt on this process (i.e end it), 
  // so we can return nicely to kernelspace
  // these are the values of esp, ebp from the sys_execute call that spawned 
  // this task
  uint32_t return_esp;
  uint32_t return_ebp;
  uint32_t return_eip;


  uint16_t uid; ///< User ID of the process
	uint16_t gid; ///< Group ID of the process
	
	char *wd; ///< Working directory
  
} task;

/*
When processing the execute system call, your kernel
must create a virtual address space for the new process. This
will involve setting up a new Page Directory with entries
corresponding to the figure shown on the right.

In other words, each task has its own page directory!
*/

extern task tasks[MAX_TASKS];

void init_tasks();
task *get_task();
task *get_task_in_running_terminal();

int32_t get_new_process_id();

uint32_t total_programs_running();

// Given a PID, this creates a new task struct for this process and initializes stdin, stdout file
// descriptors for it 
task* init_task(uint32_t pid);

int32_t find_unused_fd();


// some syscalls

/**
 * @brief fork() creates a new process by duplicating the calling process.
       The new process is referred to as the child process.  The calling
       process is referred to as the parent process.
 * 
 * @return int32_t 
 */
int32_t sys_fork();

/**
 * @brief getppid() returns the process ID of the parent of the calling
       process.  This will be either the ID of the process that created
       this process using fork(), or, if that process has already
       terminated, the ID of the process to which this process has been
       reparented
 * 
 */
int32_t sys_getpid();

/**
 * @brief A way to add something to the user stack of a process
 * Used by custom signal handler frame setup, for example
 */
int push_onto_task_stack(uint32_t *esp, uint32_t new_val);

/**
 * @brief Pushes a buffer onto user stack for a task
 * 
 * @param esp 
 * @param buf 
 * @param len 
 * @return int 
 */
int push_buf_onto_task_stack(uint32_t *esp, uint8_t *buf, uint32_t len);

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
