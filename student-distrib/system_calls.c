#include "system_calls.h"
#include "lib.h"
#include "filesystem.h"
#include "RTC.h"
#include "keyboard.h"
#include "task.h"
#include "paging.h"
#include "x86_desc.h"
#include "signal.h"
#include "terminal.h"
#include "errno.h"

/**
 * @brief This function programatically populates the jump table for system calls in the system_call_public.S file
 * That way we have fine control over which system call maps to which syscall number. Otherwise we'd have to 
 * arrange it in order in the assembly file which could get confusing
 * @param syscall_no 
 * @param handler 
 * @return uint32_t 
 */
uint32_t register_syscall(int syscall_no, syscall_handler handler) {
  if (syscall_no > NUM_SYSCALLS || syscall_no <= 0) {
    return -1;
  }

  jump_table[syscall_no] = handler;
  return 0;
}

/**
 * @brief This function will just populate the jump table by calling register_syscall a ton of times
 * 
 */
void register_all_syscalls() {
  	// ece391_halt
	syscall_register(1, sys_halt);
	// ece391_execute
	syscall_register(2, sys_execute);
	// ece391_read
	syscall_register(3, sys_read);
	// ece391_write
	syscall_register(4, sys_write);
	// ece391_open
	syscall_register(5, sys_open);
	// ece391_close
	syscall_register(6, sys_close);
	// ece391_getargs
	syscall_register(7, sys_getargs);
	// ece391_vidmap
	syscall_register(8, sys_vidmap);
	// ece391_set_handler
	syscall_register(9, sys_set_handler);
	// ece391_sigreturn
	syscall_register(10, sys_sigreturn);

  syscall_register(15, task_make_initd);

  // Process
	syscall_register(SYSCALL_FORK, sys_fork);
	syscall_register(SYSCALL_EXIT, sys_exit);
	syscall_register(SYSCALL_EXECVE, sys_execve);
	syscall_register(SYSCALL_WAITPID, sys_waitpid);
	syscall_register(SYSCALL_GETPID, sys_getpid);
	syscall_register(SYSCALL_BRK, sys_brk);
	syscall_register(SYSCALL_SBRK, sys_sbrk);

	// Signals
	syscall_register(SYSCALL_KILL, sys_kill);
	syscall_register(SYSCALL_SIGACTION, sys_sigaction);
	syscall_register(SYSCALL_SIGSUSPEND, sys_sigsuspend);
	syscall_register(SYSCALL_SIGPROCMASK, sys_sigprocmask);
}

/*
The halt system call terminates a process, returning the specified
 value to its parent process. The system call handler
itself is responsible for expanding the 8-bit argument
from BL into the 32-bit return value to the parent program’s
execute system call. Be careful not to return all 32 bits from EBX
This call should never return to the caller.
*/
int32_t sys_halt(uint8_t status)
{
  // critical code, no interrupts
  cli();

  task *current_running_task = get_task_in_running_terminal();
  uint32_t current_task_pid = current_running_task->pid;
  task *parent_task_ptr = current_running_task->parent_task;
  uint32_t parent_task_pid = parent_task_ptr->pid;

  // otherwise its closing a process and returning to another
  // close all open files associated with the task
  // de-allocate all the file descriptors of that task
  int i;
  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    // this will use the fd's jump table
    // will also deal with setting all fd values
    // like jump tables and such to 0
    // so that illegal calls cannot be made without getting errors
    // if file is not open to begin with then it won't do anything
    close(i);
  }
  // no need to do anything else, all the other task fields get over-written
  // on a second call to execute()

  // destroy the task
  tasks[current_task_pid].status = TASK_ST_DEAD;

  // also update the terminal with that info
  terminals[cur_terminal_running].num_processes_running--;

  // now if we have nothing leftover in this terminal again we know we just got rid of the shell
  // so just re-run it
  if (terminals[cur_terminal_running].num_processes_running == 0)
  {
    printf("restarting terminal\n");
    execute((uint8_t*) "shell");
  }

  printf("shutdown process %d, returning to process %d\n", current_task_pid, parent_task_pid);
  // otherwise we switch to the parent task
  map_process_mem(parent_task_pid);

  // tss cares about esp and ss, ss stays the same
  // this is the kernel esp for the parent process, which was created in execute() call
  // for the current process
  tss.esp0 = current_running_task->return_esp;

  terminals[cur_terminal_running].current_task = parent_task_ptr;

  sti();
  // return to old esp/ebp in the old task. Pass in status to eax,
  // and move the right esp and ebp values into esp and ebp.

  uint32_t reg_status = (uint32_t)status;

  // also, RETURN TO EXECUTE, where we have put a label!
  asm volatile(
      "movl %0, %%esp;"
      "movl %1, %%ebp;"
      "movl %2, %%eax;"
      "jmp execute_return_label"
      : // no outputs
      : "r"(current_running_task->return_esp), "r"(current_running_task->return_ebp), "r"(reg_status)
      : "%eax", "%esp", "%ebp");
  // make compiler happy
  return 0;
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
int32_t sys_execute(const uint8_t *command)
{

  // no interrupts when initializing a program
  cli();

  // parse the program name + Arguments
  int8_t program_name[LINE_BUFFER_MAX_SIZE + 1];
  int8_t arguments[LINE_BUFFER_MAX_SIZE + 1];

  parse_command((int8_t *)program_name, (int8_t *)arguments, (int8_t *)command);
  // printf("program name: %s, args: %s\n", program_name, arguments);

  // special commands

  // type halt to quit a process in a shell
  // uint32_t total = total_programs_running();
  if (strncmp("halt", program_name, 4) == 0 && total_programs_running != 0)
  {
    halt(0);
    return 0;
  }

  // look for program's dentry_t struct
  dentry_t program;
  if (read_dentry_by_name((uint8_t *)program_name, &program) == -1)
  {
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
  if (read_data(program.inode_number, 0, first_fourty_bytes, 40) == -1)
  {
    return -1;
  }
  if (first_fourty_bytes[0] != 0x7f || first_fourty_bytes[1] != 0x45 || first_fourty_bytes[2] != 0x4c || first_fourty_bytes[3] != 0x46)
  {
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
  printf("program entry: 0x%x\n", entry_point);
  int32_t new_pid = get_new_process_id();
  printf("running %s with process id %d\n", program_name, new_pid);
  if (new_pid == -1)
  {
    // too many processes
    printf("Too many processes, %d running already\n", MAX_TASKS);
    return -1;
  }


  // AT THIS POINT WE KNOW WE WILL RUN THE PROCESS, ALL CHECKS COMPLETE

  task *t = init_task(new_pid);

  // copy argument information and process ID into the current task.
  strncpy((int8_t *)t->arguments, arguments, strlen(arguments));

  // name the task, 32 bits because that's max size of task name (as it says in spec)
  strncpy((int8_t *)t->name_of_task, program_name, MAX_FILE_NAME_LENGTH);

  printf("program name: %s\n", program_name);
  printf("program arguments: %s\n", arguments);

  /*
  The way to get this
  working is to set up a single 4 MB page directory entry that maps virtual address 0x08000000 (128 MB) to the right
  physical memory address (either 8 MB or 12 MB). Then, the program image must be copied to the correct offset
  (0x00048000) within that page.
  */
  map_process_mem(new_pid);

  // copy the executable to the task's page in memory at offset 0x48000. Since we just mapped the memory, we can
  // now virtual address 0x08048000 will be translated into 0x800000 + 0x48000
  // AKA 8MB + 48*4KB physical, or a 0x48000 offset into the right page. Just as required.
  // Also we set length to be a really big number, I know from printing out that largest
  // exe is 5277 bytes, so 10k should work. Because read_data() will exit if done reading.
  if (read_data(program.inode_number, 0, (uint8_t *)(PROGRAM_IMAGE_VIRTUAL_ADDRESS + 0x48000), 100000) == -1)
  {
    return -1;
  }

  // mark as running status?
  tasks[new_pid].status = TASK_ST_RUNNING;

  // https://wiki.osdev.org/Context_Switching
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
  // we know that 4MB-8MB maps directly thanks to the kernel page entry we made in setup_paging()
  // also we need -4 here because 8MB virtual doesn't use the right page directory entry
  tss.esp0 = TSS_ESP_CALC(new_pid);

  /*
  Finally, when a new task is started with the execute system call,
  you’ll need to store the parent task’s PCB pointer in the child
  task’s PCB so that when the child program calls halt, you
  are able to return control to the parent task.
  */
  asm volatile(
      "movl %%ebp, %0;"
      "movl %%esp, %1;"
      : "=r"(t->return_ebp), "=r"(t->return_esp));

  // if any other tasks are running in this terminal, then set
  // that task's ptr as the parent task. Otherwise set its own
  // task ptr as its parent ptr.

  if (terminals[cur_terminal_running].num_processes_running == 0)
  {
    // then no task is running, set itself as parent task

    t->ebp = terminals[cur_terminal_running].current_task->ebp;
    t->esp = terminals[cur_terminal_running].current_task->esp;
  }
  else
  {
    // otherwise there is as process already running in this terminal
    t->ebp = 0;
    t->esp = 0;
  }

  // regardless, set the current running task FOR THE TERMINAL to this new task we made
  terminals[cur_terminal_running].current_task = t;

  // and increment the count for how many tasks we have running in said terminal
  terminals[cur_terminal_running].num_processes_running++;

  // use 'the iret method' from the above link
  change_task(entry_point);

  // return value is in eax already at this point thanks to halt()
  // so all we have to do is return from the function with leave + ret
  asm volatile(
      "execute_return_label:;"
      "leave;"
      "ret;");

  // add this to not piss off the compiler, although it functionally does nothing
  // since the assembly above will exit the function
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
int32_t sys_read(int32_t fd, void *buf, int32_t nbytes)
{
  // task *PCB_data = get_task();

  // current running process
  task *PCB_data = get_task_in_running_terminal();

  if (fd < 0 || fd > 7 || buf == 0)
  {
    return -1;
  }

  // is the fd even open?
  if ((PCB_data->fds[fd].flags & FD_IN_USE) == 0)
  {
    // its not open, don't bother
    return -1;
  }

  // pass along argumenst to the appropriate function in jump table (if exists)
  if (PCB_data->fds[fd].jump_table.read == 0)
  {
    return -1;
  }
  int32_t ret = (int32_t)(PCB_data->fds[fd].jump_table.read(fd, buf, nbytes));
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
int32_t sys_write(int32_t fd, const void *buf, int32_t nbytes)
{
  task *PCB_data = get_task_in_running_terminal();

  if (fd < 0 || fd > 7 || buf == 0)
  {
    return -1;
  }

  // is the fd even open?
  if ((PCB_data->fds[fd].flags & FD_IN_USE) == 0)
  {
    // its not open, don't bother
    return -1;
  }

  // pass along argumenst to the appropriate function if it exists
  if (PCB_data->fds[fd].jump_table.write == 0)
  {
    return -1;
  }
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
int32_t sys_open(const uint8_t *filename)
{
  // we need to look thru our filesystem for such a file
  dentry_t file;
  uint32_t ret = read_dentry_by_name(filename, &file);
  if (ret == -1)
  {
    // file not found
    return -1;
  }

  // allocate an unused file descriptor in the task
  int32_t open_fd = find_unused_fd();
  if (open_fd == -1)
  {
    // no open file descriptors for current task. Abort
    return -1;
  }

  // get current terminal's active task
  task *cur_task = get_task_in_running_terminal();

  // mark the fd for that task as in use
  cur_task->fds[open_fd].flags |= FD_IN_USE;
  // set the position back to 0 if you re-open a file
  cur_task->fds[open_fd].file_position = 0;

  if (file.file_type == 1)
  {
    // directory

    // create jump table
    jump_table_fd dir_jump_table = {
        read_dir,
        write_dir,
        open_dir,
        close_dir};
    cur_task->fds[open_fd].jump_table = dir_jump_table;

    // call the open
    if (open_dir(filename) == -1)
    {
      return -1;
    }
  }
  else if (file.file_type == 2)
  {
    // regular file

    // create jump table
    jump_table_fd file_jump_table = {
        read_file,
        write_file,
        open_file,
        close_file};
    cur_task->fds[open_fd].jump_table = file_jump_table;

    // also assign the inode value since this is a file
    cur_task->fds[open_fd].inode = file.inode_number;

    // call the open
    if (open_file(filename) == -1)
    {
      return -1;
    }
  }
  else if (file.file_type == 0)
  {
    // RTC

    // create jump table
    jump_table_fd rtc_jump_table = {
        read_RTC,
        write_RTC,
        open_RTC,
        close_RTC};
    cur_task->fds[open_fd].jump_table = rtc_jump_table;

    // call the open
    if (open_RTC(filename) == -1)
    {
      return -1;
    }
  }
  else
  {
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
int32_t sys_close(int32_t fd)
{
  if (fd == 0 || fd == 1 || fd < 0 || fd > 7)
  {
    // doesn't exist or is special
    return -1;
  }
  task *cur_task = get_task_in_running_terminal();

  // if the file descriptor is open
  if ((cur_task->fds[fd].flags & FD_IN_USE) != 0)
  {

    if (cur_task->fds[fd].jump_table.close == 0)
    {
      return -1;
    }
    // call the right close function if it exists
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
  else
  {
    // else it's not open
    return -1;
  }
  return 0;
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
int32_t sys_getargs(uint8_t *buf, int32_t nbytes)
{
  if (buf == 0)
  {
    return -1;
  }
  task *cur_task = get_task_in_running_terminal();
  if (strlen((int8_t *)&(cur_task->arguments)) + 1 > nbytes)
  {
    return -1;
  }
  strcpy((int8_t *)buf, (int8_t *)&(cur_task->arguments));

  // if no arguments (aka buf == "") also return -1
  if (strlen((int8_t*) &(cur_task->arguments)) == 0) {
    return -1;
  }
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
#define VIDMAP_ADDR 0x8500000
int32_t sys_vidmap(uint8_t **screen_start)
{
  if ((uint32_t)screen_start < PROGRAM_IMAGE_VIRTUAL_ADDRESS ||
      (uint32_t)screen_start > PROGRAM_IMAGE_VIRTUAL_ADDRESS + FOURMB)
  {
    return -1;
  }
  page_directory[34] = (uint32_t)(uint32_t)page_table | READ_WRITE_BIT | PRESENT_BIT | USER_BIT;

  page_table[0] = ((uint32_t)VIDEO) | USER_BIT | PRESENT_BIT | READ_WRITE_BIT;
  flush_tlb();

  *screen_start = (uint8_t *)(VIDMAP_ADDR);
  return VIDMAP_ADDR;
}

