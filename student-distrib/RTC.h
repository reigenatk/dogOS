#ifndef RTC_H
#define RTC_H

#include "types.h"

void init_RTC();
int set_RTC_rate(uint32_t rate_num);
void RTC_interrupt_handler();
void toggle_run();

// for the syscalls
int32_t open_RTC();
int32_t read_RTC();
int32_t write_RTC();
int32_t close_RTC();

volatile uint8_t the_flag;

#endif
