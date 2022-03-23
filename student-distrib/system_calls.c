#include "system_calls.h"

// these are the actual system calls
// since definition doubles as a declaration and a definition
// we will just omit putting the signatures into the header files
// when linker looks for this in interrupt_wrapper.S it will see these and be OK
int32_t sys_halt(uint8_t status) {
  printf("hi");
  return 0x20;
}
int32_t sys_execute(const uint8_t* command) {
  printf("hi");
  return 0x20;
}

/*
The read system call reads data from the keyboard, a file, device (RTC), or directory. 
This call returns the number
of bytes read. If the initial file position is at or beyond the end of file, 0 shall be 
returned (for normal files and the
directory). 
You should use a jump table referenced
by the task’s file array to call from a generic handler for this call into a 
file-type-specific function. This jump table
should be inserted into the file array on the open system call (see below).

*/
int32_t sys_read(int32_t fd, void* buf, int32_t nbytes) {
  task* PCB_data = (task*) calculate_task_pcb_pointer(1);
  if (fd < 0 || fd > 7 || buf == 0) {
    return -1;
  }

  // pass along argumenst to the appropriate function in jump table
  int32_t ret = PCB_data->fds[fd].jump_table.read(fd, buf, nbytes);
  return ret;
}

/*
The write system call writes data to the terminal or to a device (RTC). 
In the case of the terminal, all data should
be displayed to the screen immediately. In the case of the RTC, 
the system call should always accept only a 4-byte
integer specifying the interrupt rate in Hz, and should set the 
rate of periodic interrupts accordingly. Writes to regular
files should always return -1 to indicate failure since the file 
system is read-only. The call returns the number of bytes
written, or -1 on failure.
*/
int32_t sys_write(int32_t fd, const void* buf, int32_t nbytes) {
  task* PCB_data = (task*) calculate_task_pcb_pointer(1);
  if (fd < 0 || fd > 7 || buf == 0) {
    return -1;
  }

  // pass along argumenst to the appropriate function
  int32_t ret = PCB_data->fds[fd].jump_table.write(fd, buf, nbytes);
  return ret;
}

/*
The open system call provides access to the file system. T
he call should find the directory entry corresponding to the
named file, allocate an unused file descriptor, and set up any data 
necessary to handle the given type of file (directory,
RTC device, or regular file). If the named file does not exist or 
no descriptors are free, the call returns -1.
*/
int32_t sys_open(const uint8_t* filename) {
  // we need to look thru our filesystem for such a file
  dentry_t file;
  uint32_t ret = read_dentry_by_name(filename, &file);
  if (ret == -1) {
    // file not found
    return -1;
  }

  // allocate an unused file descriptor in the task
  uint32_t open_fd = find_unused_fd();
  if (open_fd == -1) {
    // no open file descriptors for current task. Abort
    return -1;
  }

  // mark that file descriptor as in use
  cur_task.fds[open_fd].flags |= FD_IN_USE_BIT;

  if (file.file_type == 1) {
    // directory

    // create jump table
    jump_table_fd dir_jump_table = {
      read_dir,
      write_dir,
      open_dir,
      close_dir
    };
    cur_task.fds[open_fd].jump_table = dir_jump_table;

    // also assign the inode value since this is a file
    cur_task.fds[open_fd].inode = file.inode_number;

    // call the open
    if (open_dir(filename) == -1) {
      return -1;
    }
  }
  else if (file.file_type == 2) {
    // regular file

    // create jump table
    jump_table_fd file_jump_table = {
      read_file,
      write_file,
      open_file,
      close_file
    };
    cur_task.fds[open_fd].jump_table = file_jump_table;

    // call the open
    if (open_file(filename) == -1) {
      return -1;
    }
  }
  else if (file.file_type == 0) {
    // RTC
    
    // create jump table
    jump_table_fd rtc_jump_table = {
      read_RTC,
      write_RTC,
      open_RTC,
      close_RTC
    };
    cur_task.fds[open_fd].jump_table = rtc_jump_table;

    // call the open
    if (open_RTC(filename) == -1) {
      return -1;
    }
  } 
  else {
    // unknown file type
    return -1;
  }
}

/*
The close system call closes the specified file descriptor and makes it 
available for return from later calls to open.
You should not allow the user to close the default descriptors (0 for input 
and 1 for output). Trying to close an invalid
descriptor should result in a return value of -1; successful closes should 
return 0.
*/
int32_t sys_close(int32_t fd) {
  if (fd == 0 || fd == 1 || fd < 0 || fd > 7) {
    // doesn't exist or is special
    return -1;
  }
  // check to see if this fd is open for this task
  if (cur_task.fds[fd].flags &= FD_IN_USE_BIT != 0) {
    // call the right close function
    cur_task.fds[fd].jump_table.close(fd);

    // also set the file descriptor to initial values
    cur_task.fds[fd].file_position = 0;
    cur_task.fds[fd].flags = 0; // mark it as not in use
    cur_task.fds[fd].inode = 0;

    cur_task.fds[fd].jump_table.close = 0;
    cur_task.fds[fd].jump_table.open = 0;
    cur_task.fds[fd].jump_table.read = 0;
    cur_task.fds[fd].jump_table.write = 0;
  }
  else {
    // it's not open
    return -1;
  }

}

/*
The getargs call reads the program’s command line arguments into a user-level buffer. 
Obviously, these arguments
must be stored as part of the task data when a new program is loaded. 
Here they are merely copied into user space. If
there are no arguments, or if the arguments and a terminal NULL (0-byte) 
do not fit in the buffer, simply return -1. The
shell does not request arguments, but you should probably still initialize 
the shell task’s argument data to the empty
string.
*/
int32_t sys_getargs(uint8_t* buf, int32_t nbytes) {
  if (buf == 0) {
    return -1;
  }

  if (strlen(&cur_task.arguments) + 1 > nbytes) {
    return -1;
  }
  strcpy(buf, &cur_task.arguments);
  return 0;
}

/*
The vidmap call maps the text-mode video memory into user space at a pre-set 
virtual address. Although the address
returned is always the same (see the memory map section later in this handout),
it should be written into the memory
location provided by the caller (which must be checked for validity). If the location 
is invalid, the call should return -1.
To avoid adding kernel-side exception handling for this sort of check, 
you can simply check whether the address falls
within the address range covered by the single user-level page. Note that the 
video memory will require you to add
another page mapping for the program, in this case a 4 kB page. It is not ok 
to simply change the permissions of the
video page located < 4MB and pass that address.
*/
int32_t sys_vidmap(uint8_t** screen_start) {
  // we get to define where the text-mode video memory maps into userspace
  // let's put it at 0x70000000
  uint32_t pre_set_virtual_address = 0x70000000;

  // check to see if the address passed in is within the address range of this task
  // address range of this task can be calculated if we find the process id, and then 
  // we know its 4MB page offsets from 8MB which was the kernel page

  task* PCB_data = (task*) calculate_task_pcb_pointer(1);
  uint32_t pid = PCB_data->pid;
  uint32_t start_of_memory = calculate_task_physical_address(pid);
  uint32_t end_of_memory = start_of_memory + FOURMB;
  if (screen_start < start_of_memory || screen_start > end_of_memory) {
    return -1;
  }

  // now map a new 4KB page to go from our virtual address to our physical  
}
int32_t sys_set_handler(int32_t signum, void* handler_address) {
  printf("hi");
  return 0x20;
}
int32_t sys_sigreturn(void) {
  printf("hi");
  return 0x20;
}
