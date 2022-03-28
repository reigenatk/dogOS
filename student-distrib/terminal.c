#include "terminal.h"

terminal_t t;
int cur_terminal = 0;
terminal_t terminals[NUM_TERMINALS];

// i = the terminal 
void clear_terminal_line_buffer(uint32_t terminal_num) {
  int i;
  for (i = 0; i < LINE_BUFFER_MAX_SIZE; i++) {
    terminals[terminal_num].line_buffer[i] = '\0';
  }
  terminals[terminal_num].line_buffer_idx = 0;
}

void init_terminal() {
  int i;
  for (i = 0; i < NUM_TERMINALS; i++) {
    // let current running pid of 0 mean no task is running
    terminals[i].current_running_pid = 0;
    clear_terminal_line_buffer(i);
    terminals[i].line_buffer_idx = 0;
    terminals[i].newline_received = 0;
    terminals[i].screen_x = 0;
    terminals[i].screen_y = 0;
    terminals[i].is_displayed = 0;
    int j, k;
    // clear all the screen information for each terminal
    for (j = 0; j < NUM_ROWS; j++) {
      for (k = 0; k < NUM_COLS; k++) {
        uint32_t idx = j * NUM_COLS + k;
        terminals[i].screen[idx] = ' ';
        terminals[i].screen[idx] = ATTRIB;
      }
    }
  }
  cur_terminal = 0;
  terminals[cur_terminal].is_displayed = 1;

  // execute instance of shell on this terminal
  // execute should be smart enough to assign current running process 
  // on this terminal to the pid of the newly made process for shell
  execute("shell");
}

int32_t terminal_close(int32_t fd) {
  return 0;
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
  clear_terminal_line_buffer(cur_terminal);
  // enable keyboard typing
  sti();

  while (terminals[cur_terminal].line_buffer_idx < LINE_BUFFER_MAX_SIZE- 1 && terminals[cur_terminal].newline_received != 1)
    ;
  cli();

  // once we reach here, we know either buffer was full or newline 
  // was received

  // if buffer was full, we still have to add newline
  if (terminals[cur_terminal].line_buffer_idx == LINE_BUFFER_MAX_SIZE - 1) {
    terminals[cur_terminal].line_buffer[LINE_BUFFER_MAX_SIZE - 1] = '\n';
  }

  // set enter flag to zero again
  terminals[cur_terminal].newline_received = 0;

  // copy the characters into the buffer. Need -2 because we don't wanna read 
  // the newline into the buffer. We can handle formatting later but we 
  // want our buffers to only have the characters. The newline comes from the keyboard 

  uint32_t i;
  for(i = 0; i < terminals[cur_terminal].line_buffer_idx - 1; i++) {
    ((uint8_t*)buf)[i] = terminals[cur_terminal].line_buffer[i];
  }

  // the number of characters read (not including newline)
  int32_t ret = terminals[cur_terminal].line_buffer_idx - 1;

  // memcpy(buf, cur_terminal->line_buffer, nbytes);
  clear_terminal_line_buffer(cur_terminal);
  return ret;
}

/*
In the case of the terminal, all data should
be displayed to the screen immediately. 
The call returns the number of bytes
written, or -1 on failure.
*/
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes) {
  cli();
  int32_t i;
  
  // for (i = 0; i < nbytes; i++) {
  //   putc(((int8_t*) buf)[i]);
  // }
  printf((int8_t*)buf);
  sti();
  return nbytes;
  
}
int32_t terminal_open(const uint8_t* directory) {
  return 0;
}