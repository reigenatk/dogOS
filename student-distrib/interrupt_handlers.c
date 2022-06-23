#include "interrupt_handlers.h"
#include "x86_desc.h"
#include "lib.h"
#include "signal.h"
#include "scheduler.h"

// this is standard interrupt vector for keyboard, as seen in the course notes. It's also IRQ1 on master PIC.
#define TIMER_CHIP_INTERRUPT_VECTOR 0x20
#define KEYBOARD_INTERRUPT_VECTOR 0x21 
#define RTC_INTERRUPT_VECTOR 0x28
#define SYSCALL_INTERRUPT_VECTOR 0x80 // INT 0x80

// use a macro to mass produce a bunch of functions
//spin infinitely otherwise this exception handler will get spam called
// also use #, the alternative operator, to dereference names of macro arguments

// cr2: Contains a value called Page Fault Linear Address (PFLA). When a page fault occurs, 
// the address the program attempted to access is stored in the CR2 register.

// cr3: CR3 enables the processor to translate linear addresses into physical addresses 
// by locating the page directory and page tables for the current task. 
// Typically, the upper 20 bits of CR3 become the page directory base register (PDBR), 
// which stores the physical address of the first page directory

// page fault error code: RSVD | U/S | R/W | P (bits 3-0 in that order)
#define GENERIC_INTERRUPT_HANDLER(name_of_handler) \
  void name_of_handler()                           \
  {                                                \
    cli();                                         \
    clear();                                       \
    uint32_t cr2, cr3, esp, eip, error_code;       \
    asm volatile(                                  \
        "movl %%cr2, %0;"                       \
        "movl %%cr3, %1;"                       \
        "movl %%esp, %2;"                       \
        "movl 0x8(%%esp), %3;"                       \
        "popl %4;"                                  \
        : "=r"(cr2), "=r"(cr3), "=r"(esp), "=r"(eip), "=r" (error_code) \
        );                                      \
    bluescreen();                                  \
    change_write_head(0, 0);                        \
    printf("%s", #name_of_handler);                \
    change_write_head(0, 20);                      \
    printf("cr2: 0x%x, cr3: 0x%#x, esp: 0x%x, eip: 0x%x, errorcode: 0x%x", cr2, cr3, esp, eip, error_code); \
    sti();                                         \
    while(1); \
  }

// declare all the intel defined exceptions
// GENERIC_INTERRUPT_HANDLER(divide_exception);
// GENERIC_INTERRUPT_HANDLER(debug_exception);
// GENERIC_INTERRUPT_HANDLER(nmi_interrupt);
// GENERIC_INTERRUPT_HANDLER(breakpoint_exception);
// GENERIC_INTERRUPT_HANDLER(overflow_exception);
// GENERIC_INTERRUPT_HANDLER(bound_range_exception);
// GENERIC_INTERRUPT_HANDLER(invalid_opcode_exception);
// GENERIC_INTERRUPT_HANDLER(device_not_available_exception);
// GENERIC_INTERRUPT_HANDLER(double_fault_exception);
// GENERIC_INTERRUPT_HANDLER(coprocessor_segment_overrun_exception);
// GENERIC_INTERRUPT_HANDLER(invalid_tss_exception);
// GENERIC_INTERRUPT_HANDLER(segment_not_present_exception);
// GENERIC_INTERRUPT_HANDLER(stack_fault_exception);
// GENERIC_INTERRUPT_HANDLER(general_protection_exception);
// GENERIC_INTERRUPT_HANDLER(page_fault_exception);
// #15 is intel reserved
// GENERIC_INTERRUPT_HANDLER(floating_point_error_exception);
// GENERIC_INTERRUPT_HANDLER(alignment_check_exception);
// GENERIC_INTERRUPT_HANDLER(machine_check_exception);
// GENERIC_INTERRUPT_HANDLER(simd_fp_exception);
// 20-31 are intel reserved
// 32-255 are free for user to define

void idt_make_entry(idt_desc_t *idte, void *handler, int dpl) {
	uint32_t addr32 = (uint32_t) handler;
	(*idte).val[0] = addr32 & 0x0000ffff;
	(*idte).val[1] = addr32 & 0xffff0000;
	(*idte).seg_selector = 2 * 8; // Kernel Code Segment
	(*idte).present = 1;
	(*idte).size = 1; // Always 32 bits
	(*idte).dpl = dpl;
}

void idt_make_interrupt(idt_desc_t *idte, void *handler, int dpl) {
	idt_make_entry(idte, handler, dpl);
	(*idte).reserved4 = 0;
	(*idte).reserved3 = 0;
	(*idte).reserved2 = 1;
	(*idte).reserved1 = 1;
	(*idte).reserved0 = 0;
}

void add_interrupt_handler_functions(idt_desc_t *idt) {
  // This sets the offset_31_16 and offset_15_00 fields
  idt_make_interrupt(idt + 0, divide_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 1, debug_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 2, nmi_interrupt, IDT_DPL_USER);
  idt_make_interrupt(idt + 3, breakpoint_exception, IDT_DPL_USER);
  idt_make_interrupt(idt + 4, overflow_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 5, bound_range_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 6, invalid_opcode_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 7, device_not_available_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 8, double_fault_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 9, coprocessor_segment_overrun_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 10, invalid_tss_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 11, segment_not_present_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 12, stack_fault_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 13, general_protection_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 14, page_fault_exception, IDT_DPL_KERNEL);
  // #15 is intel reserved
  idt_make_interrupt(idt + 16, floating_point_error_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 17, alignment_check_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 18, machine_check_exception, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + 19, simd_fp_exception, IDT_DPL_KERNEL);

  idt_make_interrupt(idt + TIMER_CHIP_INTERRUPT_VECTOR, pit_handler_wrapper, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + KEYBOARD_INTERRUPT_VECTOR, keyboard_handler_wrapper, IDT_DPL_KERNEL);
  idt_make_interrupt(idt + RTC_INTERRUPT_VECTOR, rtc_handler_wrapper, IDT_DPL_KERNEL);

  // Generic syscall handler 0x80
  idt_make_interrupt(idt + SYSCALL_INTERRUPT_VECTOR, syscall_handler_wrapper, IDT_DPL_USER);
}

void idt_bp_handler() {
  printf("Breakpoint\n");
}

void idt_pagefault_handler(int eip, int err, int addr) {
  clear(); 
  // bluescreen();                                                               
  change_write_head(0, 20);                      
  printf("process pagefaulted at addr: 0x%x, error code 0x%x, eip 0x%x", addr, err, eip);
  // terminate offending process
  sys_kill(get_task()->pid, SIGSEGV);
  next_scheduled_task();
}

void idt_signal_handler(int signo) {
  printf("Signal triggered with signal num %d", signo);
  sys_kill(get_task()->pid, signo);
  next_scheduled_task();
}

void idt_panic_handler(char *msg) {
  printf("Received fatal exception: \n");
	printf(msg);
	printf("PLEASE REBOOT.\n");
	while(1) {
		// asm volatile("hlt");
	}
}

// we also have to initialize all of the descriptors
void init_interrupt_descriptors(idt_desc_t *idt) {
  add_interrupt_handler_functions(idt);

  // int i;
  // // interrupt gate, not trap gate
  // for (i = 0; i < NUM_VEC; i++) {

  //   // add all the other flags
  //   // from lowest bits to high
  //   idt[i].seg_selector = KERNEL_CS; // kernel code segment
  //   idt[i].reserved4 = 0x00;
  //   idt[i].reserved3 = 0;
  //   idt[i].reserved2 = 1;
  //   idt[i].reserved1 = 1;
  //   idt[i].size = 1; // 32 bit
  //   idt[i].reserved0 = 0;
  //   idt[i].dpl = 0; // only ring 0 should be able to use this
  //   idt[i].present = 1;
  // }
  // // make the syscall descriptor dpl = 3 since syscalls have to be invokable from user
  // idt[SYSCALL_INTERRUPT_VECTOR].dpl = 3;

  // // breakpoint exception
  // idt[3].dpl = 3;
  // // overflow exception
  // idt[4].dpl = 3; 
}
