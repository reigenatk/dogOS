#include "RTC.h"
#include "i8259.h"
#include "lib.h"

// RTC is IRQ8, irq vector 0x28
// info from https://wiki.osdev.org/RTC
#define RTC_IRQ_NUM 0x8
#define RTC_IO_PORT 0x70
#define CMOS_IO_PORT 0x71
#define RTC_REG_A 0x8A
#define RTC_REG_B 0x8B
#define RTC_REG_C 0x8C

// num from 1 to 15, 0 for off
// initialize RTC Rate
uint8_t current_rate;
uint8_t is_running;

void init_RTC() {

  outb(RTC_REG_B, RTC_IO_PORT);// register B, disable NMI
  uint8_t prev = inb(CMOS_IO_PORT);
  outb(RTC_REG_B, RTC_IO_PORT);// have to do this again cuz of the read, resets read to register D
  outb(prev | 0x40, CMOS_IO_PORT);

  enable_irq(RTC_IRQ_NUM);
  set_RTC_rate(15); //15 would be 32768 / (2^(15 -1)) = 2 Hz or 2 per second
  is_running = 0;
}

void set_RTC_rate(uint8_t rate_num) {
  cli();
  if (rate_num < 1 || rate_num > 15) {
    return;
  }
  outb(RTC_REG_A, RTC_IO_PORT); // select register A, disable NMI
  uint8_t prev = inb(CMOS_IO_PORT);
  outb(RTC_REG_A, RTC_IO_PORT);
  outb((prev & 0xF0) | rate_num, CMOS_IO_PORT);

  sti();
}

// function should call test_interrupts every time it receives an interrupt
void RTC_interrupt_handler() {
  cli();
  send_eoi(RTC_IRQ_NUM);
  if (is_running == 1) {
    test_interrupts();
  }
  outb(0x0C, RTC_IO_PORT);
  inb(CMOS_IO_PORT);
  sti();
}

void toggle_run() {
  is_running = !is_running;
}