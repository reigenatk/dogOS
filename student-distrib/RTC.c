#include "RTC.h"
#include "i8259.h"
#include "lib.h"
#include "tests.h"

// RTC is IRQ8, irq vector 0x28
// info from https://wiki.osdev.org/RTC
#define RTC_IRQ_NUM 0x8
#define RTC_IO_PORT 0x70
#define CMOS_IO_PORT 0x71
#define RTC_REG_A 0x8A
#define RTC_REG_B 0x8B
#define RTC_REG_C 0x8C

#define HIGH_RTC_RATE 4096
#define RTC_X 70
#define RTC_Y 1

uint8_t is_running;
uint32_t current_RTC_rate = 2;

void init_RTC() {

  outb(RTC_REG_B, RTC_IO_PORT);// register B, disable NMI
  uint8_t prev = inb(CMOS_IO_PORT);
  outb(RTC_REG_B, RTC_IO_PORT);// have to do this again cuz of the read, resets read to register D
  outb(prev | 0x40, CMOS_IO_PORT);

  enable_irq(RTC_IRQ_NUM);
}


/*
The RTC interrupt rate should be set to a default value of 
2 Hz (2 interrupts per second) when the RTC device is opened.
For simplicity, RTC interrupts should remain on at all times.
*/
int32_t open_RTC(const uint8_t* filename) {
  // just call init_RTC()

  // set RTC to highest possible rate
  // On most machines however, the RTC interrupt rate can not go higher than 8 kHz.
  set_RTC_rate(HIGH_RTC_RATE);
  current_RTC_rate = 2;
  init_RTC();
  return 0;
}

/*
For the real-time clock (RTC), this call should always return 0, but only after an interrupt has
occurred (set a flag and wait until the interrupt handler clears it, then return 0).
*/
int32_t read_RTC(int32_t fd, void* buf, int32_t nbytes) {
  the_flag = 1;
  while (the_flag == 1)
  {
    // do nothing
  }
  return 0;
}

/*
In the case of the RTC, the system call should always accept only a 4-byte
integer specifying the interrupt rate in Hz, 
and should set the rate of periodic interrupts accordingly. 
*/
int32_t write_RTC(int32_t fd, const void* buf, int32_t nbytes) {

  // int ret = set_RTC_rate(*((uint32_t *)buf));
  // if (ret == -1)
  // {
  //   return -1;
  // }
  uint32_t val =  *((uint32_t *)buf);
  if (is_power_of_two(val) == -1 || val < 2 || val > 32768) {
    return -1;
  }
  current_RTC_rate = val;

  return nbytes;
}

int32_t close_RTC(int32_t fd) {
  // just reset RTC frequency
  current_RTC_rate = 2;
  return 0;
}

int is_power_of_two(uint32_t rate) {
  int ret = 0;
  while (rate != 1)
  {
    if (rate%2 != 0) {
      return -1;
    }
    rate >>= 1;
    ret++;
  }
  return ret;
}

// make this return a value so we know when we entered invalid frequency
int set_RTC_rate(uint32_t rate_num) {
  // valid RTC rates vary from 2^1 to 2^15 (32768)
  // the formula is frequency =  32768 >> (rate-1);
  // 15 -> 2Hz, 14 -> 4Hz, etc. 1-> 32768 Hz

  cli();
  int power_of_two = is_power_of_two(rate_num);
  if (rate_num < 2 || rate_num > 1024 || !power_of_two)
  {
    return -1;
  }
  rate_num = (16 - power_of_two);
  outb(RTC_REG_A, RTC_IO_PORT); // select register A, disable NMI
  uint8_t prev = inb(CMOS_IO_PORT);
  outb(RTC_REG_A, RTC_IO_PORT);
  outb((prev & 0xF0) | rate_num, CMOS_IO_PORT);

  sti();
  return 0;
}

void write_RTC_data() {
 // write to vid mem
  int8_t buf[10];
  itoa(rtc_test_counter, buf, 10);
  // char *addr = ((char *)VIDEO + ((RTC_X + NUM_COLS * RTC_Y) << 1));
  // *addr = buf[0];
  print_at_coordinates("real:", RTC_X, RTC_Y - 1);

  print_at_coordinates(buf, RTC_X, RTC_Y);

  int8_t buf2[10];
  itoa(rtc_virtual_counter, buf2, 10);
  // char *addr = ((char *)VIDEO + ((RTC_X + NUM_COLS * RTC_Y) << 1));
  // *addr = buf[0];
  print_at_coordinates("virt:", RTC_X, RTC_Y + 1);

  print_at_coordinates(buf2, RTC_X, RTC_Y+2);

  int8_t buf3[10];
  itoa(current_RTC_rate, buf3, 10);
  // char *addr = ((char *)VIDEO + ((RTC_X + NUM_COLS * RTC_Y) << 1));
  // *addr = buf[0];
  print_at_coordinates("rate:", RTC_X, RTC_Y + 3);

  print_at_coordinates(buf3, RTC_X, RTC_Y+4);
}

// function should call test_interrupts every time it receives an interrupt
__attribute__((interrupt)) void RTC_interrupt_handler() {
  cli();
  send_eoi(RTC_IRQ_NUM);
  if (is_running == 1) {
    test_interrupts();
  }

  rtc_test_counter++;
  // extra *2 because its rate-1
  if (rtc_test_counter % (HIGH_RTC_RATE / (current_RTC_rate*2)) == 0 && rtc_test_counter != 0) {
    rtc_virtual_counter++;
    // clear the flag for read_RTC()
    the_flag = 0;
  }

  write_RTC_data();

  outb(0x0C, RTC_IO_PORT);
  inb(CMOS_IO_PORT);

  sti();
}

void toggle_run() {
  is_running = !is_running;
}