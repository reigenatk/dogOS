#include "interrupt_handlers.h"

// this is standard interrupt vector for keyboard, as seen in the course notes. It's also IRQ1 on master PIC.
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
#define GENERIC_INTERRUPT_HANDLER(name_of_handler) \
  void name_of_handler()                           \
  {                                                \
    cli();                                         \
    clear();                                       \
    uint32_t cr2, cr3;                             \
    asm volatile(                                  \
        "movl %%cr2, %%eax;"                       \
        "movl %%eax, %0;"                          \
        "movl %%cr3, %%eax;"                       \
        "movl %%eax, %1;"                          \
        : "=r"(cr2), "=r"(cr3)                     \
        :                                          \
        : "%eax");                                 \
    bluescreen();                                  \
    change_write_head(0, 0);                       \    
    printf("%s", #name_of_handler);                \
    change_write_head(40, 0);                      \     
    printf("cr2: 0x%x, cr3: 0x%#x", cr2, cr3);        \
    sti();                                         \
    while(1); \
  }

// declare all the intel defined exceptions
GENERIC_INTERRUPT_HANDLER(divide_exception);
GENERIC_INTERRUPT_HANDLER(debug_exception);
GENERIC_INTERRUPT_HANDLER(nmi_interrupt);
GENERIC_INTERRUPT_HANDLER(breakpoint_exception);
GENERIC_INTERRUPT_HANDLER(overflow_exception);
GENERIC_INTERRUPT_HANDLER(bound_range_exception);
GENERIC_INTERRUPT_HANDLER(invalid_opcode_exception);
GENERIC_INTERRUPT_HANDLER(device_not_available_exception);
GENERIC_INTERRUPT_HANDLER(double_fault_exception);
GENERIC_INTERRUPT_HANDLER(coprocessor_segment_overrun_exception);
GENERIC_INTERRUPT_HANDLER(invalid_tss_exception);
GENERIC_INTERRUPT_HANDLER(segment_not_present_exception);
GENERIC_INTERRUPT_HANDLER(stack_fault_exception);
GENERIC_INTERRUPT_HANDLER(general_protection_exception);
GENERIC_INTERRUPT_HANDLER(page_fault_exception);
// #15 is intel reserved
GENERIC_INTERRUPT_HANDLER(floating_point_error_exception);
GENERIC_INTERRUPT_HANDLER(alignment_check_exception);
GENERIC_INTERRUPT_HANDLER(machine_check_exception);
GENERIC_INTERRUPT_HANDLER(simd_fp_exception);
// 20-31 are intel reserved
// 32-255 are free for user to define

// let's let interrupt 



void add_interrupt_handler_functions() {
  // This sets the offset_31_16 and offset_15_00 fields
  SET_IDT_ENTRY(idt[0], divide_exception);
  SET_IDT_ENTRY(idt[1], debug_exception);
  SET_IDT_ENTRY(idt[2], nmi_interrupt);
  SET_IDT_ENTRY(idt[3], breakpoint_exception);
  SET_IDT_ENTRY(idt[4], overflow_exception);
  SET_IDT_ENTRY(idt[5], bound_range_exception);
  SET_IDT_ENTRY(idt[6], invalid_opcode_exception);
  SET_IDT_ENTRY(idt[7], device_not_available_exception);
  SET_IDT_ENTRY(idt[8], double_fault_exception);
  SET_IDT_ENTRY(idt[9], coprocessor_segment_overrun_exception);
  SET_IDT_ENTRY(idt[10], invalid_tss_exception);
  SET_IDT_ENTRY(idt[11], segment_not_present_exception);
  SET_IDT_ENTRY(idt[12], stack_fault_exception);
  SET_IDT_ENTRY(idt[13], general_protection_exception);
  SET_IDT_ENTRY(idt[14], page_fault_exception);
  // #15 is intel reserved
  SET_IDT_ENTRY(idt[16], floating_point_error_exception);
  SET_IDT_ENTRY(idt[17], alignment_check_exception);
  SET_IDT_ENTRY(idt[18], machine_check_exception);
  SET_IDT_ENTRY(idt[19], simd_fp_exception);

  SET_IDT_ENTRY(idt[KEYBOARD_INTERRUPT_VECTOR], keyboard_handler_wrapper);
  SET_IDT_ENTRY(idt[RTC_INTERRUPT_VECTOR], rtc_handler_wrapper);

  // Generic syscall handler 0x80
  SET_IDT_ENTRY(idt[SYSCALL_INTERRUPT_VECTOR], syscall_handler_wrapper);
}

// we also have to initialize all of the descriptors
void init_interrupt_descriptors() {
  add_interrupt_handler_functions();

  int i;
  // interrupt gate, not trap gate
  for (i = 0; i < NUM_VEC; i++) {

    // add all the other flags
    // from lowest bits to high
    idt[i].seg_selector = KERNEL_CS; // kernel code segment
    idt[i].reserved4 = 0x00;
    idt[i].reserved3 = 0;
    idt[i].reserved2 = 1;
    idt[i].reserved1 = 1;
    idt[i].size = 1; // 32 bit
    idt[i].reserved0 = 0;
    idt[i].dpl = 0; // only ring 0 should be able to use this
    idt[i].present = 1;
  }
  // make the syscall descriptor dpl = 3 since syscalls have to be invokable from user
  idt[SYSCALL_INTERRUPT_VECTOR].dpl = 3;
}
