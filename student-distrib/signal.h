#ifndef SIGNAL_H
#define SIGNAL_H

#include "types.h"

#define SIG_DIV_ZERO 0
#define SIG_SEGFAULT 1
#define SIG_INTERRUPT 2
#define SIG_ALARM 3
#define SIG_USER1 4

enum default_behavior
{
  IGNORE,
  KILL,
};

enum default_behavior get_default_action(uint32_t signal_num);


#endif