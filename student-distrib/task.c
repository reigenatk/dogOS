#include "task.h"
#include "paging.h"



// start this with no running tasks at all. We start counting at PID = 1.
uint32_t running_process_ids[MAX_TASKS] = {0, 0, 0, 0, 0, 0};
static uint32_t next_process_num = 1;

uint32_t get_new_process_id() {
  int i = 0;
  for (; i < MAX_TASKS; i++) {
    if (running_process_ids[i] == 0) {
      // open slot
      running_process_ids[i] = next_process_num++;
      return running_process_ids[i];
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
    0, 0, 0, 0
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

uint32_t calculate_task_physical_address(uint32_t pid) {
  // need to subtract 1 because process ID starts at 1
  return KERNEL_MEM_BOTTOM + FOURMB * (pid-1);
}

// 8kb per task, going up from bottom of kernel memory (8MB)
uint32_t calculate_task_pcb_pointer(uint32_t pid) {
  return KERNEL_MEM_BOTTOM - (TASK_STACK_SIZE)*(pid);
}

task *get_task_from_pid(uint32_t pid) {
  return (task *)calculate_task_pcb_pointer(pid);
}

uint32_t find_unused_fd() {
  int i;
  for (i = 0; i < MAX_OPEN_FILES; i++) {
    if (cur_task.fds[i].flags & FD_IN_USE_BIT != 1) {
      // free file descriptor to use
      return i;
    }
  }
  // no open fds
  return -1;
}