#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "i8259.h"
#include "lib.h"
#include "RTC.h"
#include "system_calls.h"
#include "terminal.h"


void init_keyboard();
void keyboard_INT();



// we need both uppercase_char and shift_char because caps lock doesn't do the same thing as shift
typedef struct key {
  uint8_t lowercase_char; // "a"
  uint8_t uppercase_char; // "A"
  uint8_t shift_char; // For example, A -> "!"
  uint8_t caps_and_shift; // we need this because caps + shift might pick the shift char in some situations, and the lowercase char in others
  uint8_t should_display; // set to 0 if non displayable
} key;



#endif
