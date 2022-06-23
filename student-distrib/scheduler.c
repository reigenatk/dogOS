#include "scheduler.h"
#include "terminal.h"
#include "i8259.h"
#include "paging.h"
#include "task.h"
#include "signal.h"
#include "x86_desc.h"
#include "system_calls.h"

int scheduling_on_flag = 0;
static int scheduler_idx = 0; // static, meaning seen only in this file

void scheduling_start() {
  scheduling_on_flag = 1;
}

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

void pit_interrupt_handler() {
  if (scheduling_on_flag) {
    next_scheduled_task();
  }
  send_eoi(TIMER_IRQ_NUM);
}

void scheduler_update_taskregs(struct s_regs *regs) {
  task *curr_task = get_task();
  memcpy(&curr_task->regs, regs, sizeof(struct s_regs));
}

void scheduler_change_task(task* from, task* to) {
  // we gotta remap this everytime so that the addr 0x8000000 virt translates
  // to the proper physical address
  uint32_t next_task_pid = to->pid;
  map_process_mem(next_task_pid);
  
  task *from_pcb = &tasks[from->pid];
  // check for pending signals on the process we are switching from
  if (from_pcb->pending_signals) {
    // mask out the blocked signals
    sigset_t signals_pending = from_pcb->pending_signals & (~from_pcb->signal_mask);
    int i;

    // check ALL pending signals and run corresponding handlers
    for (i = 1; i < SIG_MAX; i++) {
      if (sigismember(&signals_pending, i)) {
        // mark this signal as ran
        sigdelset(&from_pcb->pending_signals, i);

        // now run the handler. this will run either default handler or custom handler
        signal_exec(from_pcb, i);

        // now the scheduler will tick below, and start execution at newly modified registers from signal_exec, which will run the handler
        // or run a syscall to exit or something like that
      }
    }
  }
  
  // change tss to reflect values of new task
  // page 104 of ULT if you wanna know more
  tss.esp0 = to->k_esp;
  tss.ss0 = KERNEL_DS;
  send_eoi(TIMER_IRQ_NUM);

  // so iret from PIT interrupt handler will never get hit?
  scheduler_iret(&(to->regs));
}

/**
 * @brief Find the next RUNNING task. Break out of while loop once we find it, and context switch to it. This just goes up the PIDs 
 * starting at 0 until we hit the max PID valuee, then loops backs and tries again
 */
void next_scheduled_task() {
  task *t = get_task();
  pid_t pid = t->pid;
  int sanity_count = 0;
  while (1) {
    switch (tasks[scheduler_idx].status) {
      case TASK_ST_RUNNING:
        break;
      case TASK_ST_SLEEP:
        // if it has a signal pending and it isn't masked out then we will run it
        if (tasks[scheduler_idx].pending_signals && (~tasks[scheduler_idx].signal_mask)) {
          break;
        }
      default:
        // otherwise, this task is not runnable. Skip to next one in list
        sanity_count++;
        scheduler_idx++;
        if (scheduler_idx >= MAX_TASKS) {
          scheduler_idx = 0;
        }
        if (sanity_count > MAX_TASKS) {
          printf("[CRITICAL] NO POSSIBLE PROCESS TO EXECUTE!");
					while (1);
        }
        continue;
    }
    break;
  }

  // actually run the task
  scheduler_change_task(t, &tasks[scheduler_idx]);
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
void next_scheduled_task_old() {
  if (terminals[cur_terminal_running].num_processes_running == 0) {
    // if no process running yet on this terminal, start shell
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
  // save the esp and ebp currently into the current task pcb
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
    // this next terminal has a process on it too. So we do a task switcH
    scheduler_change_task(cur_task, terminals[next].current_task);

  }
  
}

