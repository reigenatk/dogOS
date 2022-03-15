#ifndef INTERRUPT_HANDLERS_H
#define INTTERUPT_HANDLERS_H

#include "x86_desc.h"
#include "lib.h"

void init_interrupt_descriptors();

void keyboard_handler_wrapper();

void rtc_handler_wrapper();

void print_exception_info();

#endif
