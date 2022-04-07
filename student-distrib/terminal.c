#include "terminal.h"
#include "lib.h"
#include "system_calls.h"
#include "task.h"
#include "paging.h"


int cur_terminal_displayed = 0;
int cur_terminal_running = 1;
terminal_t terminals[NUM_TERMINALS];

// i = the terminal 
void clear_terminal_line_buffer(uint32_t terminal_num) {
  int i;
  for (i = 0; i < LINE_BUFFER_MAX_SIZE; i++) {
    terminals[terminal_num].line_buffer[i] = 0;
  }
  terminals[terminal_num].line_buffer_idx = 0;
}

void init_terminal() {
  int i;
  for (i = 0; i < NUM_TERMINALS; i++) {
    // let current running pid of 0 mean no task is running
    terminals[i].current_task = 0;
    clear_terminal_line_buffer(i);
    terminals[i].line_buffer_idx = 0;
    terminals[i].newline_received = 0;
    terminals[i].screen_x = 0;
    terminals[i].screen_y = 0;
    terminals[i].num_processes_running = 0;

    // terminal 0: 0xB9000, terminal 1 : 0xBA000, terminal 2: 0xBB000
    terminals[i].video_mem_start = (uint8_t *)VIDEO + FOURKB * (i + 1);
    // set all video memory to zero at the beginning

  }
  // spawn an instance of shell in this terminal
  // execute should be smart enough to assign current running process 
  // on this terminal to the pid of the newly made process for shell
  // cur_terminal_displayed = 0;
  // execute("shell");
}

/*
  Saves the screen state of cur_terminal_displayed into the terminal object 
*/
void save_terminal_state() {
  // save the old video memory. We do it lazily, meaning only once we are about 
  // to switch terminals do we save the state of the current one
  memcpy((int8_t *)terminals[cur_terminal_displayed].video_mem_start, (int8_t *)VIDEO, NUM_COLS * NUM_ROWS * 2);
  terminals[cur_terminal_displayed].screen_y = (uint32_t) screen_y;
  terminals[cur_terminal_displayed].screen_x = (uint32_t) screen_x;

}



/*
  Writes the screen information from terminal cur_terminal_displayed into video 
  memory
*/
void rewrite_video_state() {
  // rewrite video memory from new terminal to video mem
  memcpy((int8_t *) VIDEO, (int8_t *) terminals[cur_terminal_displayed].video_mem_start, NUM_COLS * NUM_ROWS * 2);

  // change cursor position (this will move blinking and also set screen_x and screen_y)
  change_write_head((int8_t) terminals[cur_terminal_displayed].screen_x, (int8_t) terminals[cur_terminal_displayed].screen_y);
}

/*
Eventually, when the user makes the task’s terminal active again, 
your OS must copy the screen data from the
backing store into the video memory and re-map the virtual 
addresses to point to the video memory’s physical address.

TLDR: two pieces of state we need to restore, cursor position and video mem

Also another big point: switching to a terminal should re-execute that terminal's 
active process. IF this terminal is being accessed for the first time, it 
should spawn a shell for this terminal
*/

// changes the video on screen to new_terminal_idx
void switch_terminal(uint32_t new_terminal_idx) {

  // if we are switching to current terminal we are on, just return
  if (new_terminal_idx == cur_terminal_displayed) {
    return;
  }

  // restore video map
  // map_video_mem(0);

  save_terminal_state();
  cur_terminal_displayed = new_terminal_idx;
  rewrite_video_state();
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
  clear_terminal_line_buffer(cur_terminal_running);

  sti();
  // while (get_task()->term->line_buffer_idx < LINE_BUFFER_MAX_SIZE- 1 && get_task()->term->newline_received != 1)
  //   ;
  while (terminals[cur_terminal_running].newline_received != 1);

  cli();

  // once we reach here, we know either buffer was full or newline 
  // was received

  terminals[cur_terminal_running].line_buffer[terminals[cur_terminal_running].line_buffer_idx] = '\n';

  // set enter flag to zero again
  terminals[cur_terminal_running].newline_received = 0;

  // copy the characters into the buffer. Need -2 because we don't wanna read 
  // the newline into the buffer. We can handle formatting later but we 
  // want our buffers to only have the characters. The newline comes from the keyboard 

  uint32_t i;
  int8_t* bufnew = (int8_t*) buf;
  for(i = 0; i < terminals[cur_terminal_running].line_buffer_idx+1; i++) {
    bufnew[i] = (int8_t) terminals[cur_terminal_running].line_buffer[i];
  }

  // the number of characters read (not including newline)
  int32_t ret = terminals[cur_terminal_running].line_buffer_idx + 1;

  // memcpy(buf, cur_terminal_running->line_buffer, nbytes);
  clear_terminal_line_buffer(cur_terminal_running);
  return ret;
}

/*
In the case of the terminal, all data should
be displayed to the screen immediately. 
The call returns the number of bytes
written, or -1 on failure.
*/
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes) {
  if ((int8_t*) buf == 0) {
    return -1;
  }
  int32_t i;
  if (cur_terminal_displayed == cur_terminal_running) {
    nbytes = printf(buf);
  }
  else {
    nbytes = printf_mem(buf);
  }

  // printf((int8_t*)buf);
  return nbytes;
  
}
int32_t terminal_open(const uint8_t* directory) {
  return 0;
}