#include "task.h"
#include "paging.h"
#include "filesystem.h"
#include "RTC.h"
#include "keyboard.h"
#include "terminal.h"
#include "system_calls.h"
#include "signal.h"
#include "scheduler.h"
#include "wait.h"
#include "mm/kmalloc.h"
#include "errno.h"

// All of our running tasks! Initialized to zero since its global
task tasks[MAX_TASKS];

// allocation index
uint32_t task_allocator = 0;

typedef struct s_task_ks {
	int32_t pid;
	uint8_t stack[16380]; // Empty space to fill 16kb
} __attribute__((__packed__)) task_ks_t;

// the default location for kernel stack of each program
task_ks_t *kstack = (task_ks_t *)0x800000;

void init_tasks() {
  memset(tasks, 0, sizeof(tasks));
}

uint32_t total_programs_running()
{
  uint32_t ans = 0;
  int i;
  for (i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].status == TASK_ST_RUNNING) {
      ans++;
    }
  }
  return ans;
}

// as in linux, let's make it so that a PID is never re-assigned.
int32_t get_new_process_id()
{
  uint32_t i;
  for (i = task_allocator; i < MAX_TASKS; i++) {
    if (tasks[i].status == TASK_ST_NA) {
      // then we can use this
      task_allocator = i;
      return i;
    }
  }
  // IF WE GET HERE then we are out of tasks
  return -EAGAIN;
}

task *init_task(uint32_t pid)
{

  /*
  NOTE: this is an OLD DESCRIPTION. we aren't gonna do this no more. Our tasks are instead gonna live in a array called tasks

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

  // put this task ptr at a very specific address
  task *task_pcb = (task *)calculate_task_pcb_pointer(pid);

  // initiate two open file descriptors for stdin and stdout (0 and 1)
  file_descriptor stdin, stdout;

  // stdin is read only (keyboard input)
  jump_table_fd stdin_jt = {
      .read = terminal_read,
  };

  // stdout is write only (terminal output)
  jump_table_fd stdout_jt = {
      .write = terminal_write,
  };
  stdin.file_position = 0;
  stdin.flags = 1;
  stdin.inode = 0;
  stdin.jump_table = stdin_jt;
  stdout.file_position = 0;
  stdout.flags = 1;
  stdout.inode = 0;
  stdout.jump_table = stdout_jt;

  stdin.flags |= FD_READ_PERMS;
  stdout.flags |= FD_WRITE_PERMS;

  // set everything to 0 first
  memset(task_pcb, 0, sizeof(task));

  // these should do a deep copy? IF not then just get rid of the file_descirptor stuff
  task_pcb->fds[0] = stdin;
  task_pcb->fds[1] = stdout;

  task_pcb->pid = pid;
  task_pcb->parent_task = terminals[cur_terminal_running].current_task;




  // create page table and directory for this process. we will put it at the very top
  // of memory for the process
  // task_pcb->page_directory_address = start_physical_address;
  // task_pcb->page_table_address = start_physical_address + sizeof(uint32_t) * NUM_PAGE_ENTRIES;
  // setup_paging_for_process(cur_task.page_directory_address, cur_task.page_table_address);

  return task_pcb;
}

// these go from 8MB -12MB, 12MB-16MB, (so on) in physical memory.
uint32_t calculate_task_physical_address(uint32_t pid)
{
  // need to subtract 1 because process ID starts at 1
  return KERNEL_MEM_BOTTOM + FOURMB * (pid);
}

// 8kb per task, going up from bottom of kernel memory (8MB)
uint32_t calculate_task_pcb_pointer(uint32_t pid)
{
  return KERNEL_MEM_BOTTOM - (TASK_STACK_SIZE) * (pid+1); // the +1 since otherwise it starts at pid 0 = 0x800000 which is page fault
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
task *get_task()
{
  task *ret;
  asm volatile(
      "movl %%esp, %%eax;"
      "andl $0x007FE000, %%eax;"
      "mov %%eax, %0;"
      : "=r"(ret)
      :
      : "eax");
  return ret;
}

task *get_task_in_running_terminal()
{
  return terminals[cur_terminal_running].current_task;
}

int32_t find_unused_fd()
{
  int32_t i;
  task *cur_task = terminals[cur_terminal_running].current_task;
  for (i = 0; i < 8; i++)
  {
    file_descriptor f = cur_task->fds[i];
    uint32_t flags = f.flags;
    if ((flags & FD_IN_USE) == 0)
    {
      // printf("open fd: %d\n", i);
      return i;
    }
  }
  // no open fds
  return -1;
}

/**
 * @brief  function that simulates what asm would do? This is weird lmao.
 * 
 * @param esp The value of the esp register for this task context
 * @param new_val The value we want to be at the memory location
 * @return int 
 */
int push_onto_task_stack(uint32_t* esp, uint32_t new_val) {
  *esp -= 4; // move esp for this user stack down 4
  *esp = new_val;
  return 0;
}

int push_buf_onto_task_stack(uint32_t* esp, uint8_t* buf, uint32_t len) {
  *esp -= len; // move esp for this user stack down 4
  *esp &= ~3;  // align to DWORD (last two bits zeroed)
  memcpy(esp, buf, len);
  return 0;
}

int32_t sys_fork() {
  int32_t cur_pid = sys_getpid();
  int32_t child_pid = get_new_process_id();
  if (get_new_process_id < 0) {
    // if out of PIDs
    return child_pid;
  }
  task* cur_task_ptr = tasks + cur_pid;
  task* child_task_ptr = tasks + child_pid;

  // copy task struct over then change some stuff
  memcpy(child_task_ptr, cur_task_ptr, sizeof(task));
  child_task_ptr->pid = child_pid;
  child_task_ptr->parent_pid = cur_pid;

  // allocate a new region of memory to store the same pathname? Not sure why this kmalloc is needed but OK
  child_task_ptr->wd = kmalloc(1024);
  memcpy(child_task_ptr->wd, cur_task_ptr->wd);


  // kernel stack initialization
  // loop thru all the available kernel stacks and find the first one not being used
  // its 256 btw because its 16kb per kernel stack, and we have a 4MB page to work with. So in a way this is a hard limit on the 
  // number of processes but I'm hesitant to put NUM_TASKS since that could change.
  int i;
  for (i = 0; i < 256; i++) {
    if (kstack[i].pid < 0) {
      // then we can use this kernel stack
      kstack[i].pid = child_pid;
      child_task_ptr->k_esp = &kstack[i].stack;
      break;
    }
  }
  if (i == 256) {
    // failed to allocate a kernel stack
    return -ENOMEM;
  }

  // make corresponding pages for all the memory in the old process
  child_task_ptr->pages = kmalloc(sizeof(task_ptentry_t) * cur_task_ptr->page_limit);
  
  if (!child_task_ptr->pages) {
    return -ENOMEM;
  }

  // copy over contents of all the pages
  memcpy(child_task_ptr->pages, cur_task_ptr->pages, sizeof(task_ptentry_t) * cur_task_ptr->page_limit);

  // remap all the pages to their appropriate addresses
  for (int i = 0; i < cur_task_ptr->page_limit; i++) {
    // first check if page is there
    if (!(child_task_ptr->pages[i].pt_flags & PRESENT_BIT)) {
      break;
    }

    // mark the page as copy on write if page is rdwr
    if (child_task_ptr->pages[i].pt_flags & READ_WRITE_BIT) {
      child_task_ptr->pages[i].priv_flags |= COPY_ON_WRITE;
      child_task_ptr->pages[i].priv_flags &= ~READ_WRITE_BIT;
      cur_task_ptr->pages[i].priv_flags |= COPY_ON_WRITE;
      cur_task_ptr->pages[i].priv_flags &= ~READ_WRITE_BIT;
    }
    else {
      // map the page
      if (child_task_ptr->pages[i].pt_flags & PAGE_SIZE_BIT) {
        // 4MB
        page_dir_delete_entry(child_task_ptr->pages[i].vaddr);
        page_dir_add_4MB_entry(child_task_ptr->pages[i].vaddr, child_task_ptr->pages[i].paddr, child_task_ptr->pages[i].pt_flags);
      }
      else {
        // 4kB
        page_tab_delete_entry(child_task_ptr->pages[i].vaddr);
        map_virt_to_phys(child_task_ptr->pages[i].vaddr, child_task_ptr->pages[i].paddr, child_task_ptr->pages[i].pt_flags);
      }
    }
  }

  // Return 0 to newly created process
  child_task_ptr->regs.eax = 0;

  // done with page stuff, flush tlb
  flush_tlb();
  return child_pid;
}

int32_t sys_getpid() {
  return get_task()->pid;
}

/**
 * @brief Helper function used by exit to aid in terminating a process
 * We need to dealloc pages, dynamic memory (heap), kernel stack, and change program status
 * @param t 
 */
void task_release(task* t) {
  int i;

  // program status change
  t->status = TASK_ST_DEAD;

  // dealloc pages in use by this program
  for (i = 0; i < t->page_limit; i++) {
    if (!(t->pages[i].pt_flags & PRESENT_BIT)) {
      // if page not present then end of list
      break;
    }

    // unalloc pages
    if (t->pages[i].pt_flags & PAGE_SIZE_BIT) {
			// 4MB page
			page_alloc_free_4MB(t->pages[i].paddr);
		} else {
			// 4KB page
			page_alloc_free_4KB(t->pages[i].paddr);
		}
    
    // free dynamic memory
    if (t->pages) {
      kfree(t->pages);
    }
    if (t->wd) {
      kfree(t->wd);
    }

    // free kernel stack by setting PID of the kernel stack descriptor to -1 (indicating that its free)
    ((struct s_task_ks *)(t->k_esp))->pid = -1;
  }
}

int32_t sys_exit(int32_t status) {
  int32_t cur_pid = sys_getpid();
  int32_t parent_pid = get_task()->parent_pid;
  task* cur_task_ptr = tasks + cur_pid;
  task* parent_task_ptr = tasks + parent_pid;

  // close all file descriptors
  int i;
  for (i = 0; i < MAX_OPEN_FILES; i++) {
    sys_close(i);
  }

  // run new shell process if no shell running in terminal
  // TODO: use tty here to check if shell is running or not

  // set return value to error code
  cur_task_ptr->regs.eax = status;

  // now we send either 1. SIGCHLD to parent and make the child process a zombie
  // or 2. if the handler for SIGCHLD has SA_NOCLDWAIT, then we close the current process and send a SIGCONT to parent

  if (parent_task_ptr->status == TASK_ST_SLEEP || parent_task_ptr->status == TASK_ST_RUNNING) {
    if (parent_task_ptr->sigacts[SIGCHLD].flags & SA_NOCLDWAIT) {

      scheduler_page_clear(cur_task_ptr);
      task_release(cur_task_ptr);
      sys_kill(parent_pid, SIGCONT);
    }
    else {
      // set cur task to zombie state
      cur_task_ptr->status = TASK_ST_ZOMBIE;

      // set the exit status for when we later call wait()
      // here we have two cases for which exit status we will set depending on whether or not the status indicates that cur process was terminated by signal
      if (WIFSIGNALED(status)) {
        cur_task_ptr->exit_status = status;
      }
      else {
        cur_task_ptr->exit_status = WEXITSTATUS(status) | WIFEXITED(-1);
      }
      sys_kill(parent_pid, SIGCHLD); // by default ignored
    }
  }
  else {
    // if no parent task or if parent task is dead or something then just forget about sending signal
    // and just do the typical stuff
    scheduler_page_clear(cur_task_ptr);
    task_release(cur_task_ptr);
  }

  // so at this point the task that called exit should have its status set to DEAD, therefore scheduler will never run it again
  // also parent task has been properly notified via and we are ready to call wait() to get the exit status of the dead process.
  next_scheduled_task();

  // should never hit
  return 0;
}

int32_t sys_waitpid(pid_t pid, int *wstatus, int options) {
  int i;
  if (!wstatus) {
    return -EFAULT;
  }
}