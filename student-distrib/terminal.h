#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"
#include "lib.h"

#define LINE_BUFFER_MAX_SIZE 128

typedef struct terminal_t {
  uint8_t line_buffer[LINE_BUFFER_MAX_SIZE];
  uint32_t line_buffer_idx;
  uint8_t newline_received;
} terminal_t;

extern terminal_t* cur_terminal;

void init_terminal();

int32_t terminal_close(int32_t fd);
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t terminal_open(const uint8_t* directory);

#endif