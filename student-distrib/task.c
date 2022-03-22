#include "task.h"


void init_task() {
  // initiate two open file descriptors for stdin and stdout (0 and 1)

  file_descriptor stdin, stdout;
  jump_table_fd jt;
  stdin.file_position = 0;
  stdin.flags = 0;
  stdin.inode = 0;
  stdin.jump_table = jt;
  stdout.file_position = 0;
  stdout.flags = 0;
  stdout.inode = 0;
  stdout.jump_table = jt;

  open_files[0] = stdin;
  open_files[1] = stdout;
}