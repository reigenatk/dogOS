#ifndef RTC_H
#define RTC_H

#include "types.h"

void init_RTC();
void set_RTC_rate(uint8_t rate_num);
void RTC_interrupt_handler();
void toggle_run();


#endif
