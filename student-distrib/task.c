#include "task.h"
#include "paging.h"

task cur_task = {
  .esp = 1
};

void init_task() {
  // initiate two open file descriptors for stdin and stdout (0 and 1)

  file_descriptor stdin, stdout;
  jump_table_fd jt;
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
  cur_task.fds[0] = stdin;
  cur_task.fds[1] = stdout;

  // allocate a kernel stack
  uint32_t start_of_task_stack = KERNEL_MEM_BOTTOM - TASK_STACK_SIZE;

  // set up virtual address space for this new task
  uint32_t start_physical_address = calculate_task_physical_address(0);
  virtual_to_physical_remap_directory(PROGRAM_IMAGE_VIRTUAL_ADDRESS, start_physical_address, 1, 1);
}

uint32_t calculate_task_physical_address(uint32_t task_num) {
  // need to subtract 1 because process ID starts at 1
  return KERNEL_MEM_BOTTOM + FOURMB * (task_num-1);
}

// 8kb from up per task, from bottom of kernel memory
uint32_t calculate_task_pcb_pointer(uint32_t task_num) {
  return KERNEL_MEM_BOTTOM - (TASK_STACK_SIZE)*(task_num);
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