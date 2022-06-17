/**
 * @file signal_user.h
 * @brief Creates some globally accessible variables that signal functions can jump to
 * @date 2022-06-15
 */

#ifndef SIGNAL_USER_H
#define SIGNAL_USER_H

#include "types.h"
#include "libc/sys/types.h"

#define SIGNAL_BASE_ADDR	0x10000000 ///< Base address of this page


// start of signal_user.S, we care about the address of it
extern uint32_t signal_user_base;

extern size_t size_of_signal_asm;

extern uint32_t offset_of_signal_systemcall_user;
#define signal_systemcall_user_addr ((void*) SIGNAL_BASE_ADDR + offset_of_signal_systemcall_user)

extern uint32_t offset_of_signal_user_ret;
#define sigreturn_user_addr ((void*)SIGNAL_BASE_ADDR + offset_of_signal_user_ret)

#endif

