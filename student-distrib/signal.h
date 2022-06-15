#ifndef SIGNAL_H
#define SIGNAL_H

#include "types.h"

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


/**
 * Signal handler
 * same as __signalfn_t which is void (int) in signal.h:107 (3.7)
 *	@param sig: the signal number
 */
typedef void (*sig_t)(int sig);

/// Signal set
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
	sig_t handler;
	/// Flags. See `SA_*` macro definitions
	uint32_t flags;
	/// Bitmap of signals to be masked (that is, cannot run handler again) 
  //  during execution of the handler
	sigset_t mask;
};

typedef struct sigaction sigaction_t;

/**
 * @brief System call handler for ECE391's set_handler (which is very
 * similar to real kernel's sigaction call)
 * 
 * @param signum 
 * @param handler_address 
 * @return int32_t 
 */
int32_t sys_set_handler(int32_t signum, void *handler_address);

/**
 * @brief The sigaction system call is #13 and basically swaps the sigaction
 * on the currently running task, for signal signum (since each signal number
 * has its own sigaction. Implementation:
 * https://elixir.bootlin.com/linux/v4.2/source/kernel/signal.c#L3078
 * @param signum The signum to affect
 * @param sigaction_ptr A pointer to the new sigaction struct we wanna use for said signum
 * @param oldsigaction_ptr A pointer to store the old sigaction struct
 * @return int32_t 
 */
int32_t sys_sigaction(int signum, int sigaction_ptr, int oldsigaction_ptr);

/**
 * @brief 
 * 
 * @return int32_t 
 */
int32_t sys_sigreturn(void);

// FLAGS for sigaction (we dont use SIGINFO?)
// Also as you can see we added custom signal flag for ECE391 actions
// https://elixir.bootlin.com/linux/v4.2/source/arch/x86/include/uapi/asm/signal.h#L77
/// Do not send SIGCHLD to the parent when the process is stopped
#define SA_NOCLDSTOP	0x1
/// Do not create a zombie when the process terminates
#define SA_NOCLDWAIT	0x2
/// Ignored. (Use an alternative stack for the signal handler)
#define SA_ONSTACK		0x4
/// Interrupted system calls are automatically restarted
#define SA_RESTART		0x8
/// Ignored. (Do not mask the signal while executing the signal handler)
#define SA_NODEFER		0x10
/// Reset to default action after executing the signal handler
#define SA_RESETHAND	0x20
/// Send signal number to handler in ECE391 format
#define SA_ECE391SIGNO	0x40

/// Add signal to set
#define sigaddset(set, signo) (*(set) |= (1<<(signo)))
/// Delete signal from set
#define sigdelset(set, signo) (*(set) &= ~(1<<(signo)))
/// Deselect all signals
#define sigemptyset(set) (*(set) = 0)
/// Select all signals
#define sigfillset(set) (*(set) = -1)
/// Test signal existence in set
#define sigismember(set, signo) (*(set) & (1<<(signo)))


#endif
