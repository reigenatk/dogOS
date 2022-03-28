#ifndef RTC_H
#define RTC_H

#include "types.h"

void init_RTC();
int set_RTC_rate(uint32_t rate_num);
void RTC_interrupt_handler();
void toggle_run();

// for the syscalls
int32_t open_RTC(const uint8_t* filename);
int32_t read_RTC(int32_t fd, void* buf, int32_t nbytes);
int32_t write_RTC(int32_t fd, const void* buf, int32_t nbytes);
int32_t close_RTC(int32_t fd);

volatile uint8_t the_flag;
extern uint32_t current_RTC_rate;


#endif
