#include "scheduler.h"
#include "terminal.h"
#include "i8259.h"
#include "paging.h"
#include "task.h"
#include "x86_desc.h"

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
  outb(LOWBYTE_HIGHBYTE | MODE_2, COMMAND_REGISTER);

  // low 8 bits first then high 8 bits
  outb(LOW_EIGHT_BITS & HUNDRED_HERTZ_DIVIDER, CHANNEL_0);
  outb((HUNDRED_HERTZ_DIVIDER >> 8), CHANNEL_0);

  // enable PIC to give us IRQs
  enable_irq(TIMER_IRQ_NUM);

}

void switchTasks(uint32_t terminal_to_switch_to) {
  // assumed to be running already, switch_pid != 0
  uint32_t switch_pid = terminals[terminal_to_switch_to].current_task;

  // remap the virtual memory at 0x8000000 to the right 4MB page for this task
  uint32_t start_physical_address = calculate_task_physical_address(switch_pid);
  page_directory[32] = start_physical_address | PRESENT_BIT | READ_WRITE_BIT | PAGE_SIZE_BIT | USER_BIT;

  // get the currently running and process we want to run's task pointers
  task *cur_process_task_ptr = get_task();
  task *switch_process_task_ptr = calculate_task_pcb_pointer(switch_pid);

  // always will switch to a process on a different terminal (since we have filtered
  // out in the pit interrupt handler, cases where we are staying on same terminal)
  cur_terminal_running = terminal_to_switch_to;

  // change tss to reflect values of new task
  tss.esp0 = KERNEL_MEM_BOTTOM - ((switch_pid-1) * TASK_STACK_SIZE) - 4;
  tss.ss0 = KERNEL_DS;

  // save old ebp/esp
  asm volatile(
    "movl %%esp, %0;"
    "movl %%ebp, %1;"
    : "=r"(cur_process_task_ptr->esp), "=r"(cur_process_task_ptr->ebp)
  );
  
  send_eoi(TIMER_IRQ_NUM);
  // change ebp/esp to switched tasks' values. After this we should be in 
  // the next task
  asm volatile(
    "movl %0, %%esp;"
    "movl %1, %%ebp;"
    : 
    : "r"(switch_process_task_ptr->esp), "r"(switch_process_task_ptr->ebp)
  );
  return;
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
  cli();
  if (terminals[cur_terminal_running].num_processes_running == 0) {
    // start the shell on this process

    task t;
    asm volatile(
      "movl %%esp, %0;"
      "movl %%ebp, %1;"
      : "=r"(t.ebp), "=r"(t.esp)
    );

    terminals[cur_terminal_running].current_task = &t;

    // switch terminal changes video memory basically. Do this because 
    // we are gonna execute a shell which will write some stuff. Might overwrite stuff on our current screen
    switch_terminal(cur_terminal_running);

    sti();
    execute("shell");
  }
  else {
    // otherwise there's already a process on this current terminal.
    task* cur_task = terminals[cur_terminal_running].current_task;
    asm volatile(
      "movl %%esp, %0;"
      "movl %%ebp, %1;"
      : "=r"(cur_task->ebp), "=r"(cur_task->esp)
    );

    // in round robin fashion, we will look at the next terminal
    uint32_t next = (cur_terminal_running + 1) % NUM_TERMINALS;
    if (terminals[next].num_processes_running == 0) {
      cur_terminal_running = next;
      // let another tick occur, and the next terminal will have a shell
      sti();
      return;
    }
    else {
      // this next terminal has a process on it too. So we do a task switch

      
      // map_video_mem(cur_terminal_running);

      

      // change tss to reflect values of new task
      uint32_t next_task_pid = terminals[next].current_task->pid;
      tss.esp0 = KERNEL_MEM_BOTTOM - ((next_task_pid-1) * TASK_STACK_SIZE) - 4;
      tss.ss0 = KERNEL_DS;

      map_process_mem(next_task_pid);
      
      uint32_t next_esp = terminals[next].current_task->esp;
      uint32_t next_ebp = terminals[next].current_task->ebp;

      cur_terminal_running = next;
      sti();
      // switch to next task by changing esp, ebp
      asm volatile(
        "movl %0, %%esp;"
        "movl %1, %%ebp;"
        : 
        : "r"(cur_task->esp), "r"(cur_task->ebp)
      );
    }
  }

  // cli();

  // uint32_t next = cur_terminal_displayed;
  // int i;

  // // if only one terminal running, then there is nothing to do.
  // int num_terminals_running = 0;
  // for (i = 0; i< NUM_TERMINALS; i++) {
  //   if (terminals[i].current_running_pid != 0) {
  //     num_terminals_running++;
  //   }
  // }
  // if (num_terminals_running == 0) {
  //   sti();
  //   return;
  // }

  // // otherwise try going through the other terminals, find the first one that has 
  // // an active process
  // for (i = 0; i < NUM_TERMINALS-1; i++) {
  //   next = (next + 1) % NUM_TERMINALS;
  //   if (terminals[next].current_running_pid != 0) {
  //     // then it is running a process, so we have found a suitable next process
  //     // just break
  //     break;
  //   }
  // }
  // // then switch to that task and execute it until next PIT interrupt
  // switchTasks(next);
  // sti();
  // return;
}

