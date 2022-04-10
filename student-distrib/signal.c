#include "signal.h"


enum default_behavior get_default_action(uint32_t signal_num) {
  if (signal_num == SIG_DIV_ZERO) {
    return KILL;
  }
  if (signal_num == SIG_SEGFAULT) {
    return KILL;
  }
  if (signal_num == SIG_INTERRUPT) {
    return KILL;
  }
  if (signal_num == SIG_ALARM) {
    return IGNORE;
  }
  if (signal_num == SIG_USER1) {
    return IGNORE;
  }
}