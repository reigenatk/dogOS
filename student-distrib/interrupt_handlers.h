#ifndef INTERRUPT_HANDLERS_H
#define INTERRUPT_HANDLERS_H


// #include "keyboard.h"
// #include "RTC.h"
#include "types.h"
#include "x86_desc.h"

// reference https://elixir.bootlin.com/linux/v3.0/source/arch/x86/include/asm/ptrace.h
// basically everytime we pusha. Stores all the info about a process. I think TSS stores/uses this?
// 14 entries btw, first 9 will get you the GPR (general purpose registers)
struct s_regs {
	uint32_t magic;		///< Should be 1145141919
	uint32_t edi;		///< edi saved by pusha
	uint32_t esi;		///< esi saved by pusha
	uint32_t ebp;		///< ebp saved by pusha
	uint32_t esp_k;		///< esp saved by pusha (Not used. See `esp`)
	uint32_t ebx;		///< ebx saved by pusha
	uint32_t edx;		///< edx saved by pusha
	uint32_t ecx;		///< ecx saved by pusha
	uint32_t eax;		///< eax saved by pusha
	uint32_t eip;		///< eip in iret structure
	uint32_t cs;		///< cs in iret structure
	uint32_t eflags;	///< eflags in iret structure
	uint32_t esp;		///< esp in iret structure
	uint32_t ss;		///< ss in iret structure
};

void init_interrupt_descriptors(idt_desc_t* idt);

void divide_exception();
void debug_exception();
void nmi_interrupt();
void breakpoint_exception();
void overflow_exception();
void bound_range_exception();
void invalid_opcode_exception();
void device_not_available_exception();
void double_fault_exception();
void coprocessor_segment_overrun_exception();
void invalid_tss_exception();
void segment_not_present_exception();
void stack_fault_exception();
void general_protection_exception();
void page_fault_exception();
void floating_point_error_exception();
void alignment_check_exception();
void machine_check_exception();
void simd_fp_exception();

void idt_bp_handler();

/**
 *	Page fault handler
 *
 *	The page fault handler also handles copy-on-write in addition to simply
 *	sending a SIGSEGV signal to the faulting process.
 *
 *	@param eip: faulting instruction
 *	@param err: error code
 *	@param addr: faulting address
 */
void idt_pagefault_handler(int eip, int err, int addr);


/**
 *	Delivers a signal to the current process to handle the exception
 *
 *	@param sig: the signal
 */
void idt_signal_handler(int signo);

/**
 * @brief Interrupts that should cause kernel panic will call this (comes with a message to display)
 * 
 * @param msg 
 */
void idt_panic_handler(char *msg);

// handler wrappers are global in interrupt_wrapper.S, we need to forward declare them here
void keyboard_handler_wrapper();

void syscall_handler_wrapper();

void rtc_handler_wrapper();

void pit_handler_wrapper();

// for bluescreens
void print_exception_info();

#endif
