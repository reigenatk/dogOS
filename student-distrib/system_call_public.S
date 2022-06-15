#define ASM 1

#include "ece391sysnum.h"
#include "x86_desc.h"

.data 
.globl syscall_handler_wrapper

# define the actual syscall functions as extern. ,They are defined in system_calls.c
.extern sys_halt, sys_execute, sys_read, sys_write, sys_open, sys_close, sys_getargs, sys_vidmap, sys_sethandler, sys_sigreturn

# this is a template for generic macros that will move the arguments of the syscall 
# into the defined registers that the MP specifies:

# In particular, the call number is placed in EAX, the first argument in EBX, then
# ECX, and finally EDX. No call uses more than three arguments, 
# although you should protect all of the registers from
# modification by the system call to avoid leaking 
# information to the user programs. The return value is placed in EAX
# when the call returns (not all do); a value of -1 indicates 
# an error, while others indicate some form of success.
#
#define DO_CALL(name,number)   \
.data                         ;\
.globl name                   ;\
.text                 ;\
name:                 ;\
  pushl	%ebx          ;\
  movl	$number,%eax  ;\
  movl	8(%esp),%ebx  ;\
  movl	12(%esp),%ecx ;\
  movl	16(%esp),%edx ;\
  int	$0x80         ;\
  popl	%ebx          ;\
  ret


# numbers defined in ece391sysnum.h
DO_CALL(halt,SYS_HALT)
DO_CALL(execute,SYS_EXECUTE)
DO_CALL(read,SYS_READ)
DO_CALL(write,SYS_WRITE)
DO_CALL(open,SYS_OPEN)
DO_CALL(close,SYS_CLOSE)
DO_CALL(getargs,SYS_GETARGS)
DO_CALL(vidmap,SYS_VIDMAP)
DO_CALL(set_handler,SYS_SET_HANDLER)
DO_CALL(sigreturn,SYS_SIGRETURN)

DO_CALL(sigaction, SYSCALL_SIGACTION)
DO_CALL(kill, SYSCALL_KILL)
DO_CALL(sigsuspend, SYSCALL_SIGSUSPEND)
DO_CALL(sigprocmask, SYSCALL_SIGPROCMASK)

# wrap syscall handler too
# "In particular, the call number is placed in EAX, the first argument in EBX, then
# ECX, and finally EDX. No call uses more than three arguments, 
# although you should protect all of the registers from
# modification by the system call to avoid leaking information to the user programs. 
# The return value is placed in EAX
# iff the call returns (not all do); a value of -1 indicates an error, 
# while others indicate some form of success."
# eax, edx, ecx are caller save, so ebx is callee save and must be pushed first on stack
syscall_handler_wrapper:
	cli
	push %ebx # callee save so we have to save this stuff
	push %esi
	push %edi 

	# No call uses more than three arguments, although you should protect 
	# all of the registers from modification by the system call to avoid 
	# leaking information to the user programs. So let's save the other registers
	push %ebp
	push %ds
	push %es 
	push %fs
	pushfl # save flags too

	push %edx # push arguments in reverse order onto stack
	push %ecx 
	push %ebx 

	# check if eax is in the range [1,10], if not return -1
	cmp $1, %eax 
	jl return_negative_one
	cmp $10, %eax
	jg return_negative_one

	# use eax (syscall number, 1-10) as index for jump table. Subtract 1 because it starts at 1
	sub $1, %eax
	sti
	call *jump_table(,%eax, 4) # jump to the syscall's C function
	cli
	# at this point eax should have the return value from said syscall
	jmp return
return_negative_one:
	mov $-1, %eax
return:
	addl $12, %esp # move esp back since we pushed arguments and never pop them into anything
	
	popfl
	pop %fs
	pop %es
	pop %ds 
	pop %ebp
	# pop callee save registers in reverse order
	pop %edi 
	pop %esi 
	pop %ebx
	sti
	iret # return from system call handler

jump_table:
	.long sys_halt, sys_execute, sys_read, sys_write, sys_open, sys_close, sys_getargs, sys_vidmap, sys_set_handler, sys_sigreturn

# Refer to: https://wiki.osdev.org/Getting_to_Ring_3
# or this:
# Kernel code executes at privilege level 0, while user-level code must execute at privilege level 3. The x86 processor
# does not allow a simple function call from privilege level 0 code to privilege level 3, so you must use an x86-specific
# convention to accomplish this privilege switch.

# The convention to use is the IRET instruction. Read the ISA reference manual for the details of this instruction. You
# must set up the correct values for the user-level EIP, CS, EFLAGS, ESP, and SS registers on the kernel-mode stack, and
# then execute an IRET instruction. The processor will pop the values off the stack into those registers, and by doing
# this, will perform a privilege switch into the privilege level specified by the low 2 bites of the CS register. The values
# for the CS and SS registers must point to the correct entries in the Global Descriptor Table that correspond to the
# user-mode code and stack segments, respectively. 

# The EIP you need to jump to is the entry point from bytes 24-27 of
# the executable that you have just loaded. Finally, you need to set up a user-level stack for the process. For simplicity,
# you may simply set the stack pointer to the bottom of the 4 MB page already holding the executable image. Two final
# bits: the DS register must be set to point to the correct entry in the GDT for the user mode data segment (USER DS)
# before you execute the IRET instruction (conversely, when an entry into the kernel happens, for example, through a
# system call, exception, or interrupt, you should set DS to point to the KERNEL DS segment). Finally, you will need to
# modify the TSS; this is explained in Appendix E.
.globl change_task
change_task:
	#function prologue
	push %ebp
	mov %esp, %ebp
	# it goes ds (must be USER_DS according to docs, set to KERNEL_DS when entry into kernel happens), 
	# esp, eflags, cs, entry point (EIP)

	# store entry point
	mov 8(%esp), %ebx 

	xorl %eax, %eax
	movw $USER_DS, %ax # USER_DS = 0x2B
	movw %ax, %ds # LOAD DS with USER DS!
	pushl %eax

	# stack pointer points to the bottom of the 4MB page created for the user task, NOT kernel
	# since virtual mem 0x8000000 points to the top of the right page (thanks to the page remapping we did earlier), 
	# we add 4MB aka 0x400000 which will get us to the bottom. Then the stack grows up.
	# but lefs do 0x8400000 - 4 because, 0x8400000 actually doesn't translate to the right 
	# page, its 0000 1000 01 which would be entry 33 instead of 32. So we need to 
	# put stack pointer a little lower so it still translates properly, otherwise 
	# we get "cannot access memory address at 0x8400000" which is because we don't have 
	# a page directory entry for it!
	pushl $0x83FFFFC
	pushfl
	pop %eax 
	orl $0x200, %eax # set interrupt flag to 1, enables interrupts
	pushl %eax # push flags back onto stack
	pushl $USER_CS # 0x23
 
	# entry point we stored earlier
	pushl %ebx
	iret


.globl save_ebp, save_esp 

save_esp:
	mov %esp, %eax 
	ret

save_ebp:
	mov %ebp, %eax 
	ret
