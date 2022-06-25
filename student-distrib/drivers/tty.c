#include "tty.h"

tty_t *cur_tty = NULL;
static tty_t tty_list[TTY_NUMBER];



int tty_init() {
  int i;
	// initialize all terminals to sleep
	for (i=0; i<TTY_NUMBER; ++i){
		tty_list[i].tty_status = TTY_SLEEP | TTY_BACKGROUND;
		// initialize flags, 0 for now
		tty_list[i].indev_flag = 0;
		tty_list[i].outdev_flag = 0;
		tty_list[i].flags = 0;
		tty_list[i].input_pid_waiting = 0;

		// initialize buffer
		tty_list[i].buf.index = 0;
		tty_list[i].buf.end = TTY_BUF_LENGTH - 1;
		tty_list[i].buf.flags = 0;
	}
  
	// initialize the first tty
	tty_list[0].tty_status = TTY_ACTIVE | TTY_FOREGROUND;
	tty_list[0].output_private_data = terminal_out_tty_init();
	tty_list[0].fg_proc = 1; 		// NOTE HARDCODED
	tty_list[0].root_proc = 1;		// NOTE HARDCODED
}
