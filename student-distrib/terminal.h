#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"
#include "lib.h"
#include "system_calls.h"

#define LINE_BUFFER_MAX_SIZE 128
#define NUM_TERMINALS 3

// page directory entry 0000 0010 00 -> 8
#define TERMINAL_VIDEO_VIRTUAL_ADDRESS 0x2000000

/*
In order to support the notion of a terminal, you must have a 
separate input buffer associated with each terminal. In
addition, each terminal should save the current text s
creen and cursor position in order to be able to return to 
the correctstate. Switching between terminals is equivalent to 
switching between the associated active tasks of the terminals.
*/
typedef struct terminal_t {
  // line buffer for line-buffered input
  uint8_t line_buffer[LINE_BUFFER_MAX_SIZE];
  uint32_t line_buffer_idx;

  // actual entire screen (for when we switch terminals)
  uint8_t screen[NUM_ROWS * NUM_COLS * 2];
  
  // for terminal_read, set whenever user types enter
  uint8_t newline_received;

  // since each terminal can run one task only, we store which task 
  // it is running using its pid, which we can easily find the task 
  // pointer for. 
  uint32_t current_running_pid;

  // cursor screen position in this terminal (again for switching)
  uint32_t screen_x;
  uint32_t screen_y;



} terminal_t;

// which terminal is running? Index into terminals array. This 
// needs to be extern so that other files can see which process is running
extern int cur_terminal_displayed;

extern terminal_t terminals[NUM_TERMINALS];

// called in kernel.c, starts the first terminal with an instance of shell
void init_terminal();

// called when ALT + Function key combo pressed
void switch_terminal(uint32_t new_terminal_idx);

int32_t terminal_close(int32_t fd);
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t terminal_open(const uint8_t* directory);

#endif