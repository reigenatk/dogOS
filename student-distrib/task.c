#include "task.h"
#include "paging.h"



// start this with no running tasks at all. We start counting at PID = 1.
uint32_t running_process_ids[MAX_TASKS+1] = {0, 0, 0, 0, 0, 0, 0};
static uint32_t next_process_num = 1;

int32_t get_new_process_id() {
  int i = 1;
  for (; i <= MAX_TASKS; i++) {
    if (running_process_ids[i] == 0) {
      // open slot
      running_process_ids[i] = 1;
      return i;
    }
  }
  // full
  return -1;
}

task* init_task(uint32_t pid) {

  /*
  The next piece to support tasks in your operating system is per-task data structures, for example, the process control
  block (PCB). One bit of per-task state that needs to be saved is the file array, described earlier; another is the signal
  information. You may need to store some other things in the process control block; you must figure the rest out on
  your own. 

  The final bit of per-task state that needs to be allocated is a kernel stack for each user-level program. Since
  you only need to support two tasks, you may simply place the first task’s kernel stack at the bottom of the 4 MB kernel
  page that you have already allocated. The second task’s stack can then go 8 kB above it. This way, both tasks will have
  8 kB kernel stacks to use when inside the kernel. Each process’s PCB should be stored at the top of its 8 kB stack, and
  the stack should grow towards them. Since you’ve put both stacks inside the 4 MB page, there is no need to “allocate”
  memory for the process control block. To get at each process’s PCB, you need only AND the process’s ESP register
  with an appropriate bit mask to reach the top of its 8 kB kernel stack, which is the start of its PCB.
  
  In other words, this points to an address in kernel memory, we cast it to type (task*) so 
  that we can access the different values in a task struct
  */
  task* task_pcb = (task*) calculate_task_pcb_pointer(pid);

  // initiate two open file descriptors for stdin and stdout (0 and 1)
  file_descriptor stdin, stdout;
  jump_table_fd jt = {
    .read = terminal_read,
    .open = terminal_open,
    .write = terminal_write,
    .close = terminal_close,
  };
  stdin.file_position = 0;
  stdin.flags = 1;
  stdin.inode = 0;
  stdin.jump_table = jt;
  stdout.file_position = 0;
  stdout.flags = 1;
  stdout.inode = 0;
  stdout.jump_table = jt;

  stdin.flags |= FD_READ_PERMS;
  stdout.flags |= FD_WRITE_PERMS;

  // these should do a deep copy? IF not then just get rid of the file_descirptor stuff
  task_pcb->fds[0] = stdin;
  task_pcb->fds[1] = stdout;


  // create page table and directory for this process. we will put it at the very top 
  // of memory for the process
  // task_pcb->page_directory_address = start_physical_address;
  // task_pcb->page_table_address = start_physical_address + sizeof(uint32_t) * NUM_PAGE_ENTRIES;
  // setup_paging_for_process(cur_task.page_directory_address, cur_task.page_table_address);

  return task_pcb;
}

// these go from 8MB -12MB, 12MB-16MB, (so on) in physical memory. 
uint32_t calculate_task_physical_address(uint32_t pid) {
  // need to subtract 1 because process ID starts at 1
  return KERNEL_MEM_BOTTOM + FOURMB * (pid-1);
}

// 8kb per task, going up from bottom of kernel memory (8MB)
uint32_t calculate_task_pcb_pointer(uint32_t pid) {
  return KERNEL_MEM_BOTTOM - (TASK_STACK_SIZE)*(pid);
}

/*
To get at each process’s PCB, you need only AND the process’s ESP register
with an appropriate bit mask to reach the top of its 8 kB kernel stack,
which is the start of its PCB.

BIT MASK calculation: since 0x400000 - 0x800000 (4mb - 8mb) is mapped exactly to the 
same address range due to our kernel page entry, we can work directly with these addresses
The pcb's will live at 8kb offsets, so 0x7FE000, 0x7FC000, .. 0x7F2000, etc. So we can just 
take current ESP, and & it with 0xFFE000 and it should work. We don't have to have any 
higher bits because we know it only goes down from 0x7FE000. And we don't care about 
last 3 bits because they are at 0x2000 offsets, and we want zeros for last 3 bits. 
And we do E instead of F for the third bit because the values are always even
*/
task *get_task() {
  task *ret;
  asm volatile(
      "movl %%esp, %%eax;"
      "andl $0x007FE000, %%eax;"
      "mov %%eax, %0;"
      : "=r"(ret)
      : 
      : "eax"
    );
  return ret;
}

int32_t find_unused_fd() {
  int32_t i;
  task *cur_task = get_task();
  for (i = 0; i < 8; i++) {
    file_descriptor f = cur_task->fds[i];
    uint32_t flags = f.flags;
    if ((flags & FD_IN_USE) == 0) {
      // printf("open fd: %d\n", i);
      return i;
    }
  }
  // no open fds
  return -1;
}