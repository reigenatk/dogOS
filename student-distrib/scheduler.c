#include "scheduler.h"

// https://wiki.osdev.org/Programmable_Interval_Timer#Operating_Modes
// 1.193182 MHz, we want 100 Hz, since 10 ms = (1 / 100) sec and 
// so its 100/sec or 100Hz. Then 50ms = (5/100) / sec or 20Hz.
// and the spec says we want 10-50 ms
// 16 bit reload value, we are going to use Channel 0 at 0x40
// so set the counter to be 11931.82 - 59659.1
// let's go with 100Hz so we need 11932 roughly

void init_PIT() {
  cli();
  // Typically, OSes and BIOSes use mode 3 (see below) for 
  // PIT channel 0 to generate IRQ 0 timer ticks, but some use mode 2 instead, 
  // to gain frequency accuracy (frequency = 1193182 / reload_value Hz).
  outb(LOWBYTE_HIGHBYTE | MODE_3, COMMAND_REGISTER);

  // low byte then high
  outb(LOW_EIGHT_BITS & HUNDRED_HERTZ_DIVIDER, CHANNEL_0);
  outb(HIGH_EIGHT_BITS & HUNDRED_HERTZ_DIVIDER, CHANNEL_0);

  // enable PIC to give us IRQs
  enable_irq(TIMER_IRQ_NUM);
  sti();
}

/*
Lastly, keep in mind that even though a process 
can be interrupted in either user mode or in kernel mode (while waiting
in a system call). After the interrupt, the processor 
will be in kernel mode, but the data saved onto the stack depends
on the state before the interrupt. Each process should 
have its own kernel stack, but be careful not to implicitly assume
either type of transition.
*/
void pit_interrupt_handler() {
  send_eoi(TIMER_IRQ_NUM);
  uint32_t next = cur_terminal_displayed;
  int i;
  // try going through the other terminals, find the first one that has 
  // an active process
  for (i = 0; i < NUM_TERMINALS; i++) {
    next = (next + 1) % NUM_TERMINALS;
    if (terminals[next].current_running_pid != 0) {
      // then it is running a process, so pre-empt it
      break;
    }
  }
  // if no other terminals are running then it will return itself
  printf("hi");
}

void switchTasks(uint32_t switch_pid) {

  // remap the virtual memory
  uint32_t start_physical_address = calculate_task_physical_address(switch_pid);
  page_directory[32] = start_physical_address | PRESENT_BIT | READ_WRITE_BIT | PAGE_SIZE_BIT | USER_BIT;

  task *cur_process_task_ptr = get_task();
  task *switch_process_task_ptr = calculate_task_pcb_pointer(switch_pid);

  // we could be switching into a process that is on the current terminal, or 
  // one that is on a different terminal. We need to differentiate between the two cases

  // change tss to reflect values of new task
  tss.esp0 = KERNEL_MEM_BOTTOM - ((switch_pid-1) * TASK_STACK_SIZE) - 4;
  tss.ss0 = KERNEL_DS;

  // save old ebp/esp
  asm volatile(
    "movl %%esp, %0;"
    "movl %%ebp, %1;"
    : "=r"(cur_process_task_ptr->esp), "=r"(cur_process_task_ptr->ebp)
  );

  // change ebp/esp to switched tasks' values
  asm volatile(
    "movl %0, %%esp;"
    "movl %1, %%ebp;"
    : 
    : "r"(switch_process_task_ptr->esp), "r"(switch_process_task_ptr->ebp)
  );

}