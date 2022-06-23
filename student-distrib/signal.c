#include "signal.h"
#include "signal_user.h"
#include "errno.h"
#include "task.h"
#include "paging.h"
#include "lib.h"
#include "ece391sysnum.h"
#include "libc/sys/wait.h"

int ece391_signal_support[] = {
  SIGFPE,
  SIGSEGV,
  SIGINT,
  SIGALRM,
  SIGUSR1
};

// messages to print to user for each signal
char *signal_names[] = {
	"Unknown signal: 0\n",
	"Hangup: 1\n",
	"Interrupt: 2\n",
	"Quit: 3\n",
	"Illegal instruction: 4\n",
	"Trace/BPT trap: 5\n",
	"Abort trap: 6\n",
	"EMT trap: 7\n",
	"Floating point exception: 8\n",
	"Killed: 9\n",
	"Bus error: 10\n",
	"Segmentation fault: 11\n",
	"Bad system call: 12\n",
	"Broken pipe: 13\n",
	"Alarm clock: 14\n",
	"Terminated: 15\n",
	"Urgent I/O condition: 16\n",
	"Suspended (signal): 17\n",
	"Suspended: 18\n",
	"Continued: 19\n",
	"Child exited: 20\n",
	"Stopped (tty input): 21\n",
	"Stopped (tty output): 22\n",
	"I/O possible: 23\n",
	"Cputime limit exceeded: 24\n",
	"Filesize limit exceeded: 25\n",
	"Virtual timer expired: 26\n",
	"Profiling timer expired: 27\n",
	"Window size changes: 28\n",
	"Information request: 29\n",
	"User defined signal 1: 30\n",
	"User defined signal 2: 31\n",
	"" // dummy
};

int32_t sys_sigprocmask(int how, int setp, int oldsetp) {

  // save old value if not null
  if (oldsetp) {
    *(sigset_t *)oldsetp = get_task()->signal_mask;
  }
  if (setp) {
    switch (how) {
      case SIG_BLOCK:
        get_task()->signal_mask |= *(sigset_t *) setp;
        break;
      case SIG_UNBLOCK:
        get_task()->signal_mask &= ~(*(sigset_t *) setp);
        break;
      case SIG_SETMASK:
        get_task()->signal_mask = *(sigset_t *) setp;
        break;
      default:
        return -EINVAL;
    }

    // always let SIGKILL and SIGSTOP thru though, can't block those signals.
    sigdelset(&get_task()->signal_mask, SIGKILL);
    sigdelset(&get_task()->signal_mask, SIGSTOP);
  }

}

int32_t sys_sigsuspend(const sigset_t *mask) {
  if (mask) {
    // set mask using sigprocmask syscall (probably bad practice)
    sys_sigprocmask(SIG_SETMASK, mask, 0);
  }
  // set the status of the task to sleep
  get_task()->status = TASK_ST_SLEEP;
}

int32_t sys_sigaction(int signum, int sigaction_ptr, int oldsigaction_ptr) {
  if (signum > SIG_MAX || signum < 0) {
    return -EINVAL;
  }
  // get current running task pid, offset it into task list (recall that all our tasks are in a tasks array, not kernel pcb memory)
  task* t = tasks + get_task()->pid;

  // intermediate sigaction ptr to help with moving stuff around
  sigaction_t* intermediate;

  // first save old sigaction
  if (oldsigaction_ptr) {
    intermediate = (sigaction_t*) oldsigaction_ptr;
    memcpy(intermediate, &t->sigacts[signum], sizeof(struct sigaction));
  }

  // write new sigaction to task
  if (sigaction_ptr) {
    intermediate =(sigaction_t*) &t->sigacts[signum];
    memcpy(intermediate, (sigaction_t*) sigaction_ptr, sizeof(struct sigaction));
  }

  return 0;
}

void signals_init() {
  uint32_t addr = 0;
  alloc_4kb_mem(&addr);

  // add that address to the page table
  map_virt_to_phys(SIGNAL_BASE_ADDR, addr, PRESENT_BIT | READ_WRITE_BIT | USER_BIT | GLOBAL_BIT);
  flush_tlb();
  // finally we must memcpy our contents from the signal_user.S file to this newly allocated page
  memcpy(SIGNAL_BASE_ADDR, &(signal_user_base), size_of_signal_asm);
}

int32_t sys_kill(pid_t pid, int sig) {
  if (pid <= 0 || sig <= 0 || sig >= SIG_MAX) {
    return -EINVAL;
  }
  // Send the signal to the process
  // which is same as just adding the signal to the process' "signals" mask (pending signals)
  sigaddset(&tasks[pid].signals, sig);
}

/**
 * @brief Set the up frame, preparing for execution of a custom signal handler
 * https://elixir.bootlin.com/linux/latest/source/arch/ia64/kernel/signal.c#L226
 * @param proc 
 * @param sig 
 */
void setup_frame(task* proc, int sig) {
  // eip
  push_onto_task_stack(&proc->regs.esp, proc->regs.eip);
  // hw context (push first 9, aka magic + 8 General purpose registers)
  push_buf_onto_task_stack(&proc->regs.esp, (uint8_t *)&proc->regs, 9 * sizeof(uint32_t));
  push_onto_task_stack(&proc->regs.esp, proc->regs.eflags);

  // push original mask and update the process to not allow certain signals as specified by this handler
  push_onto_task_stack(&proc->regs.esp, proc->signal_mask);
  proc->signal_mask = proc->sigacts[sig].mask & (~(SIGKILL | SIGSTOP)); // must always be able to kill and stop

  // push signal num
  push_onto_task_stack(&(proc->regs.esp), sig);

  // push return address (which runs sigreturn)
  uint32_t esp = push_onto_task_stack(&proc->regs.esp, (uint32_t) sigreturn_user_addr);
  
  // finally we must run the handler
  proc->regs.eip = (uint32_t) proc->sigacts[sig].handler;
}

void signal_exec(task* proc, int sig) {
  struct sigaction a = proc->sigacts[sig];

  // IF DEFAULT HANDLER, EXECUTE DEFAULT HANDLER
  switch ((int) a.handler) {
    // check if handler is any of the preset 3
    case ((int)SIGHANDLER_DEFAULT):
      signal_exec_default(proc, sig);
      return;
    case ((int)SIGHANDLER_IGNORE):
      signal_handler_ignore(proc, sig);
      return;
  }

  // NOT default handler... custom handler instead. this is more complicated
  setup_frame(proc, sig);

}


void signal_exec_default(task *proc, int sig) {
  switch (sig) {
    case SIGCHLD:
		case SIGURG:
		case SIGCONT:
		case SIGIO:
		case SIGWINCH:
		case SIGINFO:
    case SIGALRM:
    case SIGUSR1:
    case SIGUSR2:
      signal_handler_ignore(proc, sig);
      break;
    case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
      signal_handler_stop(proc, sig);
      break;
    default:
      signal_handler_terminate(proc, sig);
  }
}

/**
 *	Handle signal by simply discarding the signal.
 *
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_handler_ignore(task *proc, int sig) {
  // do nothing
}

void signal_handler_terminate(task *proc, int sig) {
  // we do this by directly changing the registers in the task state

  // this calls exit syscall with return code -200 (terminated due to signal)
  proc->regs.eax = SYSCALL__EXIT;
  proc->regs.ebx = sig | WIFSIGNALED(-1); // return code?
	proc->regs.eip = (uint32_t) signal_systemcall_user_addr;
}

void signal_handler_stop(task *proc, int sig) {
  proc->status = TASK_ST_SLEEP;
  proc->exit_status = sig | WIFSTOPPED(-1);
}



int32_t ece391_sys_set_handler(int32_t signum, void *handler_address)
{
  // internally call sigaction which is more robust version of this
  // we only have 5 signals in ECE391 so check that first
  if (signum < 0 || signum > 4) {
    // fun fact return negative cuz generally positive codes mean success
    return -EINVAL;
  }
  // map it to actual signal number
  signum = ece391_signal_support[signum];
  struct sigaction s_act;

  // change signal handler
  s_act.handler = (sig_t) handler_address;
  // empty mask then add our signal to it
  sigemptyset(&s_act.mask);
  sigaddset(&s_act.mask, signum);
  s_act.flags = SA_ECE391SIGNO;

  return sys_sigaction(signum, (int) &s_act, 0);
}

int32_t ece391_sys_sigreturn(void)
{
  printf("hi");
  return 0x20;
}

