#include "ece391sysnum.h"
#define ASM 1

.data 
.globl syscall_handler_wrapper

# define the actual syscall functions as extern. ,They are defined in system_calls.c
.extern sys_halt, sys_execute, sys_read, sys_write, sys_open, sys_close, sys_getargs, sys_vidmap, sys_sethandler, sys_sigreturn

#define DO_CALL(name,number)   \
.data                         ;\
.globl name                   ;\
.text                 ;\
name:                 ;\
  PUSHL	%EBX          ;\
	MOVL	$number,%EAX  ;\
	MOVL	8(%ESP),%EBX  ;\
	MOVL	12(%ESP),%ECX ;\
	MOVL	16(%ESP),%EDX ;\
	INT	$0x80         ;\
	POPL	%EBX          ;\
	RET


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
	push %ebx # callee save so we have to save this stuff
	push %esi
	push %edi 

	push %edx # push arguments in reverse order onto stack
	push %ecx 
	push %ebx 

	# check if eax is in the range [1,10], if not return -1
	cmp $1, %eax 
	jl return_negative_one
	cmp $10, %eax
	jg return_negative_one

	# use eax (syscall number, 1-10) as index for jump table
	sub $1, %eax
	call *jump_table(,%eax, 4)
	pop %ebx 
	pop %esi 
	pop %edi 
	jmp return_zero
	iret

return_negative_one:
	mov $-1, %eax
	jmp return
return_zero:
	mov $0, %eax
	jmp return
return:
	# pop callee save registerse
	pop %ebx 
	pop %esi 
	pop %edi 
	iret

jump_table:
	.long sys_halt, sys_execute, sys_read, sys_write, sys_open, sys_close, sys_getargs, sys_vidmap, sys_set_handler, sys_sigreturn
