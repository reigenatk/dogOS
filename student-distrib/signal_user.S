# a file for defining global symbols related to signals, accessible from userspace applications

.globl offset_of_signal_systemcall_user, offset_of_signal_user_ret, signal_user_base

signal_user_base:
  .long 0x69696969

# use this to kill a program (after SIGKILL, or a default signal handler with action kill) 
systemcall_user:
  int $0x80

# this will do the same job as a sigreturn would, which is to restore hardware context before
# going back to kernelspace. This is after a custom signal handler has finished execution.
signal_user_ret:
	# Teardown stack for handler
	addl	$4, %esp # is this the signal number? We don't care so we go over it.
  # Restore signal mask
	movl	$29, %eax # sigprocmask
	movl	$3, %ebx # SIG_SETMASK
	movl	%esp, %ecx # Original mask
	xorl	%edx, %edx # No oldset
	int	$0x80
	addl	$4, %esp # Teardown original mask
	// Restore previous execution status
	popfl
	addl	$4, %esp # Teardown magic (remember that we pushed 9 entries but we dont want the magic number
	movl	12(%esp), %eax # Get ESP_K (ECE391 workaround)
	movl	%eax, 28(%esp) # Set to EAX (set the return value of the user function to saved esp?)
	popal # pop hw context back into place
	ret

# make sure this is the first one, its the size of entire signal_user page.
size_of_signal_asm:
   .long size_of_signal_asm - signal_user_base

offset_of_signal_systemcall_user:
  .long systemcall_user - signal_user_base

offset_of_signal_user_ret:
  .long signal_user_ret - signal_user_base