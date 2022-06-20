
#define ASM 1

# get all the signal numbers
#define SIGHUP		$1	///< terminal line hangup
#define SIGINT		$2	///< interrupt program
#define SIGQUIT		$3	///< quit program
#define SIGILL		$4	///< illegal instruction
#define SIGTRAP		$5	///< trace trap
#define SIGABRT		$6	///< abort program (formerly SIGIOT)
#define SIGEMT		$7	///< emulate instruction executed
#define SIGFPE		$8	///< floating-point exception
#define SIGKILL		$9	///< kill program
#define SIGBUS		$10	///< bus error
#define SIGSEGV		$11	///< segmentation violation
#define SIGSYS		$12	///< non-existent system call invoked
#define SIGPIPE		$13	///< write on a pipe with no reader
#define SIGALRM		$14	///< real-time timer expired
#define SIGTERM		$15	///< software termination signal
#define SIGURG		$16	///< urgent condition present on socket
#define SIGSTOP		$17	///< stop (cannot be caught or ignored)
#define SIGTSTP		$18	///< stop signal generated from
#define SIGCONT		$19	///< continue after stop
#define SIGCHLD		$20	///< child status has changed
#define SIGTTIN		$21	///< background read attempted from control terminal
#define SIGTTOU		$22	///< background write attempted to control terminal
#define SIGIO		$23	///< I/O is possible on a descriptor
#define SIGXCPU		$24	///< cpu time limit exceeded
#define SIGXFSZ		$25	///< file size limit exceeded
#define SIGVTALRM	$26	///< virtual time alarm
#define SIGPROF		$27	///< profiling timer alarm
#define SIGWINCH	$28	///< Window size change
#define SIGINFO		$29	///< status request from keyboard
#define SIGUSR1		$30	///< User defined signal 1
#define SIGUSR2		$31	///< User defined signal 2
#define SIG_MAX		$32	///< Total number of signals

#define REG_MAGIC $1145141919

# for popping the error code that is pushed onto stack by processor
TMPVAL:
	.long	0

.globl interrupt_register_info

interrupt_register_info:
	.long 0

# some of these exceptions map to certain signals. That is, the interrupt handler runs, then it should push a signal num to the stack
# and call a special signal interrupt handler. The exception to signal mapping can be found here 
# https://elixir.bootlin.com/linux/v3.14/source/arch/x86/kernel/traps.c#L214
.globl divide_exception # SIGFPE
.globl debug_exception
.globl nmi_interrupt
.globl breakpoint_exception
.globl overflow_exception # SIGSEGV
.globl bound_range_exception # SIGSEGV
.globl invalid_opcode_exception # SIGILL
.globl device_not_available_exception
.globl double_fault_exception
.globl coprocessor_segment_overrun_exception # SIGFPE
.globl invalid_tss_exception # SIGSEGV
.globl segment_not_present_exception # SIGBUS
.globl stack_fault_exception # SIGBUS 
.globl general_protection_exception
.globl page_fault_exception
.globl floating_point_error_exception
.globl alignment_check_exception # SIGBUS
.globl machine_check_exception
.globl simd_fp_exception

idt_int_msg_de:
	.string "Divide Error at %x\n"
idt_int_msg_debug:
	.string "Debug Exceptions\n"
idt_int_msg_nmi:
	.string "NMI\n"
idt_int_msg_bp:
	.string "Breakpoint\n"
idt_int_msg_of:
	.string "Overflow\n"
idt_int_msg_br:
	.string "Bound Range Exceeded at %x\n"
idt_int_msg_ud:
	.string "Undefined Opcode at %x\n"
idt_int_msg_nm:
	.string "No Math Coprocessor at %x\n"
idt_int_msg_df:
	.string "Double Fault\n"
idt_int_msg_ts:
	.string "Invalid TSS at %x (err=%x)\n"
idt_int_msg_np:
	.string "Segment Not Present at %x (err=%x)\n"
idt_int_msg_ss:
	.string "Stack-Segment Fault at %x (err=%x)\n"
idt_int_msg_gp:
	.string "General Protection Fault at %x (err=%x)\n"
idt_int_msg_mf:
	.string "Math Fault at %x\n"
idt_int_msg_ac:
	.string "Alignment Check at %x\n"
idt_int_msg_mc:
	.string "Machine Check\n"
idt_int_msg_xf:
	.string "SIMD Floating Point Exception at %x\n"
idt_int_msg_reserved:
	.string "Reserved Interrupt\n"

divide_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl SIGFPE

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

debug_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

  pushl $idt_int_msg_debug

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_panic_handler
	addl $8, %esp
	popal
	iret

nmi_interrupt:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info
	pushl $idt_int_msg_nmi

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_panic_handler
	addl $8, %esp
	popal
	iret

breakpoint_exception:
	pusha
	pushl REG_MAGIC

	movl %esp, interrupt_register_info
	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_bp_handler
	addl $4, %esp
	popal
	iret

overflow_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info
	pushl SIGSEGV

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

bound_range_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl SIGSEGV

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

invalid_opcode_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl SIGILL

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

device_not_available_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl SIGFPE

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

double_fault_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl $idt_int_msg_df

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_panic_handler
	addl $8, %esp
	popal
	iret

coprocessor_segment_overrun_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl SIGFPE

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

invalid_tss_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl SIGSEGV

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

segment_not_present_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl SIGBUS

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

stack_fault_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl SIGBUS

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

general_protection_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl	SIGSEGV

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

page_fault_exception:
	popl TMPVAL # the error code
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	# get faulting addr (in cr2)
	movl %cr2, %eax
	pushl %eax 

	# get error code
	movl TMPVAL, %eax 
	andl $0xf, %eax 
	pushl %eax 

	# get EIP (not sure why 44, shouldnt it be 9+1+1+1 = 12*4 = 48?)
	pushl 44(%esp)

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_pagefault_handler
	addl $16, %esp
	popal
	iret

floating_point_error_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	push SIGFPE 
	
	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_signal_handler
	addl $8, %esp
	popal
	iret

alignment_check_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl $idt_int_msg_ac

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_panic_handler
	addl $8, %esp
	popal
	iret

machine_check_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl $idt_int_msg_mc

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_panic_handler
	addl $8, %esp
	popal
	iret

simd_fp_exception:
	pusha
	pushl REG_MAGIC
	movl %esp, interrupt_register_info

	pushl $idt_int_msg_xf

	pushl interrupt_register_info
	call scheduler_update_taskregs
	addl $4, %esp

	call idt_panic_handler
	addl $8, %esp
	popal
	iret

.globl keyboard_handler_wrapper, rtc_handler_wrapper, pit_handler_wrapper

# wouldn't work unless I added an IRET. Needed to do this because otherwise it would 
# generate general protection exception. 
# https://wiki.osdev.org/Interrupt_Service_Routines- just add an ASM wrapper so 
# the function prologue (leave, ret) doesn't get called. We only need IRET here
keyboard_handler_wrapper:
	cli											
	call keyboard_INT
	sti						
	iret									

rtc_handler_wrapper:			
  cli			
	call RTC_interrupt_handler																
	sti
	iret	

pit_handler_wrapper:		
	cli
	call pit_interrupt_handler										
	sti
	iret	

# #define INTEL_WRAPPER_FUNC(wrapper_name, name) \
# .globl wrapper_name														 \
# wrapper_name: 																 \
# 	call name														  			 \
# 	iret						
	
# INTEL_WRAPPER_FUNC(divide_exception_wrapper, divide_exception)
# INTEL_WRAPPER_FUNC(debug_exception_wrapper, debug_exception)
# INTEL_WRAPPER_FUNC(nmi_interrupt_wrapper, nmi_interrupt)
# INTEL_WRAPPER_FUNC(breakpoint_exception_wrapper, breakpoint_exception)
# INTEL_WRAPPER_FUNC(overflow_exception_wrapper, overflow_exception)
# INTEL_WRAPPER_FUNC(bound_range_exception_wrapper, bound_range_exception)
# INTEL_WRAPPER_FUNC(invalid_opcode_exception_wrapper, invalid_opcode_exception)
# INTEL_WRAPPER_FUNC(device_not_available_exception_wrapper, device_not_available_exception)
# INTEL_WRAPPER_FUNC(double_fault_exception_wrapper, double_fault_exception)
# INTEL_WRAPPER_FUNC(coprocessor_segment_overrun_exception_wrapper, coprocessor_segment_overrun_exception)
# INTEL_WRAPPER_FUNC(invalid_tss_exception_wrapper, invalid_tss_exception)
# INTEL_WRAPPER_FUNC( segment_not_present_exception_wrapper, segment_not_present_exception)
# INTEL_WRAPPER_FUNC( stack_fault_exception_wrapper, stack_fault_exception)
# INTEL_WRAPPER_FUNC( general_protection_exception_wrapper, general_protection_exception)
# INTEL_WRAPPER_FUNC( page_fault_exception_wrapper, page_fault_exception)
# INTEL_WRAPPER_FUNC( floating_point_error_exception_wrapper, floating_point_error_exception)
# INTEL_WRAPPER_FUNC( alignment_check_exception_wrapper, alignment_check_exception)
# INTEL_WRAPPER_FUNC( machine_check_exception_wrapper, machine_check_exception)
# INTEL_WRAPPER_FUNC( simd_fp_exception_wrapper, simd_fp_exception)
