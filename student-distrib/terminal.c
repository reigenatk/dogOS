#include "terminal.h"

terminal_t t;
terminal_t* cur_terminal = &t;

void init_terminal() {
  cur_terminal->line_buffer_idx = 0;
  cur_terminal->newline_received = 0;
}

int32_t terminal_close(int32_t fd) {

}

void clear_terminal_buffer() {
  int i;
  for (i = 0; i < LINE_BUFFER_MAX_SIZE; i++) {
    cur_terminal->line_buffer[i] = '\0';
  }
  cur_terminal->line_buffer_idx = 0;
}

/*
Terminal read only returns when the enter key is pressed and should always add the newline character at the
end of the buffer before returning. Remember that the 128 character limit includes the newline character.

Terminal read should be able to handle buffer overflow (User types more than 127 characters) and situations
where the size sent by the user is smaller than 128 characters.

. If the user fills up one line by typing and hasn’t typed 128 characters yet, you should roll over to the next line.
Backspace on the new line should go back to the previous line as well.
*/
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes) {
  // clear terminal buffer ( can read in max 127 chars cuz we need newline)
  clear_terminal_buffer();
  // enable keyboard typing
  sti();

  while (cur_terminal->line_buffer_idx < LINE_BUFFER_MAX_SIZE- 1 && cur_terminal->newline_received != 1)
    ;
  cli();

  // once we reach here, we know either buffer was full or newline 
  // was received

  // if buffer was full, we still have to add newline
  if (cur_terminal->line_buffer_idx == LINE_BUFFER_MAX_SIZE - 1) {
    cur_terminal->line_buffer[LINE_BUFFER_MAX_SIZE - 1] = '\n';
  }

  // set enter flag to zero again
  cur_terminal->newline_received = 0;

  // copy the characters into the buffer
  uint32_t i;
  for(i = 0; i < cur_terminal->line_buffer_idx; i++) {
    ((uint8_t*)buf)[i] = cur_terminal->line_buffer[i];
  }

  // the number of characters read = the position of the line buffer index
  int32_t ret = cur_terminal->line_buffer_idx;

  // memcpy(buf, cur_terminal->line_buffer, nbytes);
  clear_terminal_buffer();
  return ret;
}

/*
In the case of the terminal, all data should
be displayed to the screen immediately. 
The call returns the number of bytes
written, or -1 on failure.
*/
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes) {
  uint32_t i;
  
  for (i = 0; i < nbytes; i++) {
    putc(((uint8_t*) buf)[i]);
  }
  return nbytes;
}
int32_t terminal_open(const uint8_t* directory) {

}