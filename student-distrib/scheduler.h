#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "lib.h"


#define HUNDRED_HERTZ_DIVIDER 11932
#define CHANNEL_0 0x40
#define COMMAND_REGISTER 0x43
#define HIGH_EIGHT_BITS 0xFF00
#define LOW_EIGHT_BITS 0xFF
#define MODE_3 0x06
#define MODE_2 0x04
#define LOWBYTE_HIGHBYTE 0x30
#define TIMER_IRQ_NUM 0
/*
Until this point, task switching has been done by either 
executing a new task or by halting an existing one and returning
to the parent task. By adding a scheduler, your OS will 
actively preempt a task in order to switch to the next one. Your
OS scheduler should keep track of all tasks and schedule 
a timer interrupt every 10 to 50 milliseconds in order to
switch to the next task in a round-robin fashion.

When adding a scheduler, it is important to keep in mind 
that tasks running in an inactive terminal should not write to
the screen. In order to enforce this rule, a remapping of 
virtual memory needs to be done for each task. Specifically,
the page tables of a task need to be updated to have a task
 write to non-display memory rather than display memory
when the task is not the active one (see the previous section
*/

void init_PIT();
void pit_interrupt_handler();

#endif