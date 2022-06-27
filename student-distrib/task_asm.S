.globl task_kernel_process_iret


USER_CS:
	.long	0x0023
USER_DS:
	.long	0x002B

# this is just doing change task to the "task_kernel_process" label in signal_user.S
task_kernel_process_iret:
	movl	USER_DS, %eax
	movw	%ax, %ds
	pushl 	%eax
	pushl 	$0x10001000	// NOTE BRUTAL STACK HERE
	pushf
	pushl 	USER_CS
	movl 	task_kernel_process_offset, %eax
	addl	$0x10000000, %eax
	pushl	%eax
	iret
