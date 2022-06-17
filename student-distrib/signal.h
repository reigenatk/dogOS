#ifndef SIGNAL_H
#define SIGNAL_H

#include "libc/sys/types.h"
#include "types.h"
#include "task.h"
#include "libc/signal.h"

/**
 * @brief System call handler for ECE391's set_handler (which is very
 * similar to real kernel's sigaction call)
 * 
 * @param signum 
 * @param handler_address 
 * @return int32_t 
 */
int32_t ece391_sys_set_handler(int32_t signum, void *handler_address);


/**
 * @brief Not used. Instead we link to some assembly in signal_user that will get the same job done (pops stuff off of signal stack frame)
 * 
 * @return int32_t 
 */
int32_t ece391_sys_sigreturn();

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
 * @brief sigprocmask() is used to fetch and/or change the signal mask of
       the calling thread.  The signal mask is the set of signals whose
       delivery is currently blocked for the caller
 * 
 * @param how three values possible, SIG_SETMASK (3) is probably most common (3)
 * @param setp new value of signal mask to set
 * @param oldsetp previous value of signal mask stored here if non-null
 * @return int32_t 
 */
int32_t sys_sigprocmask(int how, int setp, int oldsetp);

/**
 * @brief sigsuspend() temporarily replaces the signal mask of the calling
       thread with the mask given by mask and then suspends the thread
       until delivery of a signal whose action is to invoke a signal
       handler or to terminate a process
 * https://man7.org/linux/man-pages/man2/sigsuspend.2.html
 */
int32_t sys_sigsuspend(const sigset_t *mask);

/**
 * @brief Sends ANY signal to any process or group of proesses
 * 
 * @param pid 
 * @param sig 
 * @return int32_t 
 */
int32_t sys_kill(pid_t pid, int sig);

// forward declare this
struct task;

/**
 *	Invoke signal handler. Called by scheduler, right before returning to userspace. 
 *	Checks for whether or not the handler is a default handler
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_exec(task* proc, int sig);

/**
 *	Invoke the default signal handler. This will run one of the helper methods depending
 *	on what kind of signal it is. There are three main classes of actions we implement:
 *  ignore, stop and terminate
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_exec_default(task *proc, int sig);

/**
 *	Default SIGCHLD handler
 *
 *	@param proc: the process
 */
void signal_handler_child(task *proc);

/**
 *	Handle signal by simply discarding the signal.
 *
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_handler_ignore(task *proc, int sig);

/**
 *	Handle signal by terminating process. Will also print an error message
 *
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_handler_terminate(task *proc, int sig);

/**
 *	Handle signal by putting the process to sleep.
 *
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_handler_stop(task *proc, int sig);

/**
 * @brief Initializes a page for the signal_user methods 
 * 
 */
void signals_init();

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
