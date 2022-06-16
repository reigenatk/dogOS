/**
 * @file signal_user.h
 * @brief Creates some globally accessible variables that signal functions can jump to
 * @date 2022-06-15
 */

#ifndef SIGNAL_USER_H
#define SIGNAL_USER_H

#include "types.h"

#define PROC_USR_BASE	0x8000000 ///< Base address of this page

extern uint32_t offset_of_signal_systemcall_user;
#define signal_systemcall_user_addr ((void*)PROC_USR_BASE + offset_of_signal_systemcall_user)

extern uint32_t offset_of_signal_user_ret;
#define sigreturn_user_addr ((void*)PROC_USR_BASE + offset_of_signal_user_ret)

#endif

