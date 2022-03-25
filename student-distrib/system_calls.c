#include "system_calls.h"

// these are the actual system calls
// since definition doubles as a declaration and a definition
// we will just omit putting the signatures into the header files
// when linker looks for this in interrupt_wrapper.S it will see these and be OK
int32_t sys_halt(uint8_t status) {
  printf("hi");
  return 0x20;
}

/*
The execute system call attempts to load and execute a new program, handing off the processor to the new program
until it terminates. The command is a space-separated sequence of words. The first word is the file name of the
program to be executed, and the rest of the command—stripped of leading spaces—should be provided to the new
program on request via the getargs system call. The execute call returns -1 if the command cannot be executed,
for example, if the program does not exist or the filename specified is not an executable, 256 if the program dies by an
exception, or a value in the range 0 to 255 if the program executes a halt system call, in which case the value returned
is that given by the program’s call to halt.

*/
int32_t sys_execute(const uint8_t* command) {

  // no interrupts
  cli();

  // parse the program name + Arguments
  uint8_t program_name[MAX_FILE_NAME_LENGTH+1];
  uint8_t arguments[LINE_BUFFER_MAX_SIZE+1];

  parse_command(program_name, arguments, command);
  // printf("program name: %s, args: %s", program_name, arguments);

  // look for program's dentry_t struct
  dentry_t program;
  if (read_dentry_by_name(program_name, &program) == -1) {
    // program not found
    return -1;
  }

  // check that this is an executable
  /*
  In this file, a header that occupies the first 40 bytes gives information 
  for loading and starting
  the program. The first 4 bytes of the file represent a “magic number” 
  that identifies the file as an executable. These
  bytes are, respectively, 0: 0x7f; 1: 0x45; 2: 0x4c; 3: 0x46. If the 
  magic number is not present, the execute system
  call should fail. 
  */
  uint8_t first_fourty_bytes[40];
  if (read_data(program.inode_number, 0, first_fourty_bytes, 40) == -1) {
    return -1;
  }
  if (first_fourty_bytes[0] != 0x7f || first_fourty_bytes[1] != 0x45 
  || first_fourty_bytes[2] != 0x4c || first_fourty_bytes[3] != 0x46) {
    return -1;
  }

  /*
  The other important bit of information that you need 
  to execute programs is the entry point into the
  program, i.e., the virtual address of the first 
  instruction that should be executed. This information is stored as a 4-byte
  unsigned integer in bytes 24-27 of the executable, 
  and the value of it falls somewhere near 0x08048000 
  for all programs we have provided to you. When processing 
  the execute system call, your code should make a note of the entry
  point, and then copy the entire file to memory starting 
  at virtual address 0x08048000. It then must jump to the entry
  point of the program to begin execution.
  */
  uint32_t entry_point = *((uint32_t *)(first_fourty_bytes + 24));
  printf("program entry: 0x%x", entry_point);
  uint32_t new_pid = get_new_process_id();
  if (new_pid == -1) {
    return -1;
  }
  task* t = init_task(new_pid);

  // copy argument information and process ID into the current task.
  strncpy(t->arguments, arguments, strlen(arguments));
  t->pid = new_pid;

  /*
  The way to get this
  working is to set up a single 4 MB page directory entry that maps virtual address 0x08000000 (128 MB) to the right
  physical memory address (either 8 MB or 12 MB). Then, the program image must be copied to the correct offset
  (0x00048000) within that page.
  */
  
  uint32_t start_physical_address = calculate_task_physical_address(new_pid);

  // 0x08000000 = 128 MB, and since we take first 10 bits its 0000 | 1000 | 00, aka 32th entry
  // in the page directory table. We need this to be a 4MB page, and also a user page (cuz its a user process' memory)
  page_directory[32] = start_physical_address | PRESENT_BIT | READ_WRITE_BIT | PAGE_SIZE_BIT | USER_BIT;
  // must flush to inform the TLB of new changes to paging structures. Because 
  // otherwise the processor will use the (wrong) page cache
  flush_tlb();

  // copy the executable to the task's page in memory at offset 0x48000. Since we just mapped the memory, we can 
  // now virtual address 0x08048000 will be translated into 0x800000 + 0x48000 
  // AKA 8MB + 48*4KB physical, or a 0x48000 offset into the right page. Just as required.
  // Also we set length to be a really big number, I know from printing out that largest 
  // exe is 5277 bytes, so 10k should work. Because read_data() will exit if done reading.
  if (read_data(program.inode_number, 0, (uint8_t*) (PROGRAM_IMAGE_VIRTUAL_ADDRESS + 0x48000), 100000) == -1) {
    return -1;
  }

  // the important fields are SS0 and
  // ESP0. These fields contain the stack segment and stack pointer that the x86 will put into SS and ESP when performing
  // a privilege switch from privilege level 3 to privilege level 0 (for example, when a user-level program makes a system
  // call, or when a hardware interrupt occurs while a user-level program is executing). These fields must be set to point to
  // the kernel’s stack segment and the process’s kernel-mode stack, respectively. Note that when you start a new process,
  // just before you switch to that process and start executing its user-level code, you must alter the TSS entry to contain
  // its new kernel-mode stack pointer. This way, when a privilege switch is needed, the correct stack will be set up by the
  // x86 processor
  tss.ss0 = KERNEL_DS;

  // This is kernel stack pointer. We want this to point at 8MB physical, then 8MB-8kb physical, etc.
  // we know that 8MB maps to 8MB thanks to the kernel page entry we made in setup_paging()
  tss.esp0 = KERNEL_MEM_BOTTOM - ((new_pid-1) * TASK_STACK_SIZE);

  // use 'the iret method' from the above link
  change_task(entry_point);

  return 0;
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
  task *cur_task = get_task();
  cur_task->fds[open_fd].flags |= FD_IN_USE_BIT;

  if (file.file_type == 1) {
    // directory

    // create jump table
    jump_table_fd dir_jump_table = {
      read_dir,
      write_dir,
      open_dir,
      close_dir
    };
    cur_task->fds[open_fd].jump_table = dir_jump_table;

    // also assign the inode value since this is a file
    cur_task->fds[open_fd].inode = file.inode_number;

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
    cur_task->fds[open_fd].jump_table = file_jump_table;

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
    cur_task->fds[open_fd].jump_table = rtc_jump_table;

    // call the open
    if (open_RTC(filename) == -1) {
      return -1;
    }
  } 
  else {
    // unknown file type
    return -1;
  }

  // return newly allocated fd
  return open_fd;
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
  task* cur_task = get_task();

  // check to see if this fd is open for this task
  if (cur_task->fds[fd].flags &= FD_IN_USE_BIT != 0) {
    // call the right close function
    cur_task->fds[fd].jump_table.close(fd);

    // also set the file descriptor to initial values
    cur_task->fds[fd].file_position = 0;
    cur_task->fds[fd].flags = 0; // mark it as not in use
    cur_task->fds[fd].inode = 0;

    cur_task->fds[fd].jump_table.close = 0;
    cur_task->fds[fd].jump_table.open = 0;
    cur_task->fds[fd].jump_table.read = 0;
    cur_task->fds[fd].jump_table.write = 0;
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
  task *cur_task = get_task();
  if (strlen(&(cur_task->arguments)) + 1 > nbytes) {
    return -1;
  }
  strcpy(buf, &(cur_task->arguments));
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
  // so put in english, we get to access the video memory via 0x8000000 
  // because we make a page directory entry + page table entry that maps 
  // this virtual address to this physical address. You can run through the address translation 
  // to verify this is the case

  // we get to define where the text-mode video memory maps into userspace
  // we arbitrarily choose 0x09000000, since we know program memory is at 0x08000000
  // so this is above any address that a program would use (0x08000000 - 0x08400000)
 //  and won't interfere with any previous mappings we have already.

  virtual_to_physical_remap_usertable((uint32_t) 0x09000000, (uint32_t) VIDEO, 1);
}
int32_t sys_set_handler(int32_t signum, void* handler_address) {
  printf("hi");
  return 0x20;
}
int32_t sys_sigreturn(void) {
  printf("hi");
  return 0x20;
}
