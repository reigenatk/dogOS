#include "scheduler.h"
#include "terminal.h"
#include "i8259.h"
#include "paging.h"
#include "task.h"
#include "x86_desc.h"
#include "system_calls.h"

// https://wiki.osdev.org/Programmable_Interval_Timer#Operating_Modes
// 1.193182 MHz, we want 100 Hz, since 10 ms = (1 / 100) sec and 
// so its 100/sec or 100Hz. Then 50ms = (5/100) / sec or 20Hz.
// and the spec says we want 10-50 ms
// 16 bit reload value, we are going to use Channel 0 at 0x40
// so set the counter to be 11931.82 - 59659.1
// let's go with 100Hz so we need 11932 roughly

void init_PIT() {

  // Typically, OSes and BIOSes use mode 3 (see below) for 
  // PIT channel 0 to generate IRQ 0 timer ticks, but some use mode 2 instead, 
  // to gain frequency accuracy (frequency = 1193182 / reload_value Hz).
  outb(0x36, COMMAND_REGISTER);

  // low 8 bits first then high 8 bits
  outb(LOW_EIGHT_BITS & HUNDRED_HERTZ_DIVIDER, CHANNEL_0);
  outb((HUNDRED_HERTZ_DIVIDER >> 8), CHANNEL_0);

  // enable PIC to give us IRQs
  enable_irq(TIMER_IRQ_NUM);

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
  
  if (terminals[cur_terminal_running].num_processes_running == 0) {
    // start the shell on this process

    task t;
    asm volatile(
      "movl %%esp, %0;"
      "movl %%ebp, %1;"
      : "=r"(t.esp), "=r"(t.ebp)
      :
      : "memory"
    );

    terminals[cur_terminal_running].current_task = &t;

    // switch terminal changes video memory basically. Do this because 
    // we are gonna execute a shell which will write some stuff. Might overwrite stuff on our current screen
    switch_terminal(cur_terminal_running);

    // send eoi so that we can receive another scheduling interrupt
    // to switch to another process
    send_eoi(TIMER_IRQ_NUM);
    execute((uint8_t*) "shell");
  }
  
  // so above we start a shell if we dont already have one. Then we 
  // gotta switch to the next task
  task* cur_task = terminals[cur_terminal_running].current_task;
  asm volatile(
    "movl %%esp, %0;"
    "movl %%ebp, %1;"
    : "=r"(cur_task->esp), "=r"(cur_task->ebp)
  );

  // in round robin fashion, we will look at the next terminal
  uint32_t next = (cur_terminal_running + 1) % NUM_TERMINALS;
  if (terminals[next].num_processes_running == 0) {
    cur_terminal_running = next;
    // let another tick occur, and the next terminal will have a shell
    // but we dont need to do context switch because there is no context
    send_eoi(TIMER_IRQ_NUM);  
    return;
  }
  else {
    // this next terminal has a process on it too. So we do a task switch    

    // change tss to reflect values of new task
    uint32_t next_task_pid = terminals[next].current_task->pid;
    tss.esp0 = KERNEL_MEM_BOTTOM - ((next_task_pid-1) * TASK_STACK_SIZE) - 4;
    tss.ss0 = KERNEL_DS;

    // we gotta remap this everytime so that the addr 0x8000000 virt translates
    // to the proper physical address
    map_process_mem(next_task_pid);
    
    uint32_t next_esp = terminals[next].current_task->esp;
    uint32_t next_ebp = terminals[next].current_task->ebp;

    cur_terminal_running = next;
    send_eoi(TIMER_IRQ_NUM);
    // switch to next task by changing esp, ebp
    asm volatile(
      "movl %0, %%esp;"
      "movl %1, %%ebp;"
      : 
      : "r"(next_esp), "r"(next_ebp)
    );
  }
  
}

