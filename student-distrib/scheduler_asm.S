

scheduler_static_space:
	.long	0

scheduler_iret:
	// need return addr, cs, eflags, esp, ss
	// parameter is the address of the structure of regs
	// in order of edi esi ebp esp ebx edx ecx eax

	movl	4(%esp), %esp
	movl	%esp, %eax
	addl	$36, %eax
	movl	%eax, scheduler_static_space

	addl	$4, %esp

	popal
	// normalize esp from previously saved space
	movl	scheduler_static_space, %esp

	iret