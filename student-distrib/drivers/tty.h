#ifndef TTY_H
#define TTY_H

#include "../errno.h"
#include "../task.h"
#include "../signal.h"

#define	TTY_SLEEP 			0x0 		///< tty flag, means tty is not in use
#define TTY_ACTIVE 			0x1			///< tty flag, means tty is in use
#define TTY_FOREGROUND 		0x2 		///< tty flag, means tty is foreground
#define TTY_BACKGROUND 		0x0 		///< tty flag, means tty is background

#define TTY_NUMBER			4 			///< maximum number of tty
#define TTY_BUF_LENGTH		128 		///< maximum length of tty buffer

#define TTY_FG_ECHO		 	0x0 		///< tty echo any key press to the terminal
#define TTY_FG_NECHO		0x1 		///< tty doesn't echo key press

#define TTY_BUF_ENTER		0x1 		///< buffer flag flag for there is an enter in the buffer

#define TTY_DRIVER_NAME_LENGTH	32 		///< maximum length of in/out driver name length

/**
 *	tty buffer structure stores all information about the buffer of a tty
 * 	including flags, current index, end of buffer, and the buffer data,
 * 	note that the buffer is a wrapped around structure
 */
typedef struct s_tty_buffer{
	uint8_t 	buf[TTY_BUF_LENGTH]; 	///< buffer data
	uint8_t 	flags; 					///< buffer flag
	uint32_t 	index; 					///< index of next empty spot
	uint32_t 	end; 					///< end of the buffer
} tty_buf_t;

/**
 *	tty structure stores all the information of a tty
 */
typedef struct s_tty{
	uint32_t		tty_status;		///< tty status
	uint32_t 		indev_flag; 	///< flag and feature for input
	uint32_t 		outdev_flag; 	///< flag and feature for output
	uint32_t		flags; 			///< flags and attributes for tty
	uint32_t 		fg_proc;		///< process pid currently running at foreground of this tty
	uint32_t 		root_proc;		///< first process born with this tty
	struct s_tty_buffer	buf; 			///< tty buffer

	uint32_t 		input_pid_waiting; 		///< pid of the process waiting for input, 0 for none
	void* 			input_private_data; 	///< input private data, memory allocated by input driver
	void* 			output_private_data;	///< output private data, memory allocated by output driver

	// not implemented
/*	int (*tty_write)(uint8_t* buf, uint32_t* size); ///< output function pointer
	int (*tty_in_open)(struct s_tty* tty); 	///< open input driver
	int (*tty_out_open)(struct s_tty* tty); ///< open output driver*/
} tty_t;

int tty_init();

/// Current active TTY
extern tty_t* cur_tty;

#endif
