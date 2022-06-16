#ifndef INTERRUPT_HANDLERS_H
#define INTERRUPT_HANDLERS_H


// #include "keyboard.h"
// #include "RTC.h"
#include "types.h"

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

void init_interrupt_descriptors();

// handler wrappers are global in interrupt_wrapper.S, we need to forward declare them here
void keyboard_handler_wrapper();

void syscall_handler_wrapper();

void rtc_handler_wrapper();

void pit_handler_wrapper();

// for bluescreens
void print_exception_info();

#endif
