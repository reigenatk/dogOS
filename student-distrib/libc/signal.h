#ifndef LIBC_SIGNAL_H
#define LIBC_SIGNAL_H

#include "../types.h"

// from https://chromium.googlesource.com/chromiumos/docs/+/master/constants/signals.md
#define SIGHUP		1	///< terminal line hangup
#define SIGINT		2	///< interrupt program
#define SIGQUIT		3	///< quit program
#define SIGILL		4	///< illegal instruction
#define SIGTRAP		5	///< trace trap
#define SIGABRT		6	///< abort program (formerly SIGIOT)
#define SIGEMT		7	///< emulate instruction executed
#define SIGFPE		8	///< floating-point exception
#define SIGKILL		9	///< kill program
#define SIGBUS		10	///< bus error
#define SIGSEGV		11	///< segmentation violation
#define SIGSYS		12	///< non-existent system call invoked
#define SIGPIPE		13	///< write on a pipe with no reader
#define SIGALRM		14	///< real-time timer expired
#define SIGTERM		15	///< software termination signal
#define SIGURG		16	///< urgent condition present on socket
#define SIGSTOP		17	///< stop (cannot be caught or ignored)
#define SIGTSTP		18	///< stop signal generated from
#define SIGCONT		19	///< continue after stop
#define SIGCHLD		20	///< child status has changed
#define SIGTTIN		21	///< background read attempted from control terminal
#define SIGTTOU		22	///< background write attempted to control terminal
#define SIGIO		23	///< I/O is possible on a descriptor
#define SIGXCPU		24	///< cpu time limit exceeded
#define SIGXFSZ		25	///< file size limit exceeded
#define SIGVTALRM	26	///< virtual time alarm
#define SIGPROF		27	///< profiling timer alarm
#define SIGWINCH	28	///< Window size change
#define SIGINFO		29	///< status request from keyboard
#define SIGUSR1		30	///< User defined signal 1
#define SIGUSR2		31	///< User defined signal 2
#define SIG_MAX		32	///< Total number of signals

#ifndef ASM

// default signal handlers
// https://elixir.bootlin.com/linux/v4.2/source/include/uapi/asm-generic/signal-defs.h#L23
// btw these are the values 0, 1 and -1 cast to function ptr
#define SIGHANDLER_IGNORE ((void(*)(int)) 1)
#define SIGHANDLER_DEFAULT ((void(*)(int)) 0)
#define SIGHANDLER_ERR ((void(*)(int)) -1)

// options for argument "how" in sigprocmask 
#define SIG_BLOCK	1	///< Block signals in set
#define SIG_UNBLOCK	2	///< Unblock signals in set
#define SIG_SETMASK	3	///< Set mask to set

/**
 * Signal handler
 * same as __signalfn_t which is void (int) in signal.h:107 (3.7)
 *	@param sig: the signal number
 */
typedef void (*sig_t)(int sig);

/// Signal set (1 for signal OK to come thru, 0 for blocked)
typedef uint32_t sigset_t;

/**
 *	Signal Handling behavior descriptor
 *  Specifies the signal handler, as well as which flags to mask and 
 *  stuff like that
 */
struct sigaction {
	/**
	 *	The signal handler
	 *
	 *	@param sig: the signal number
	 */
	sig_t handler; // the handler function pointer
	/// Flags. See `SA_*` macro definitions
	uint32_t flags;
	/// Bitmap of signals to be masked (that is, cannot run handler again) 
  //  during execution of the handler
	sigset_t mask;
};

typedef struct sigaction sigaction_t;

#endif // ASM

#endif