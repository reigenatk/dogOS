#ifndef INTERRUPT_HANDLERS_H
#define INTTERUPT_HANDLERS_H

#include "x86_desc.h"
#include "lib.h"
#include "scheduler.h"
// #include "keyboard.h"
// #include "RTC.h"

void init_interrupt_descriptors();

// handler wrappers are global in interrupt_wrapper.S, we need to forward declare them here
void keyboard_handler_wrapper();

void syscall_handler_wrapper();

void rtc_handler_wrapper();

// for bluescreens
void print_exception_info();

#endif
