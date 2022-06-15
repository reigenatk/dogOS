#include "signal.h"
#include "errno.h"
#include "task.h"
#include "lib.h"


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



int32_t sys_sigaction(int signum, int sigaction_ptr, int oldsigaction_ptr) {
  if (signum > SIG_MAX || signum < 0) {
    return -EINVAL;
  }
  // get current running task
  task* t = get_task();

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


int32_t sys_set_handler(int32_t signum, void *handler_address)
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

  sys_sigaction(signum, (int) &s_act, 0);
}

int32_t sys_sigreturn(void)
{
  printf("hi");
  return 0x20;
}

