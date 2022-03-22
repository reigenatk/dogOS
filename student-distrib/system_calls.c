#include "system_calls.h"

// these are the actual system calls
// since definition doubles as a declaration and a definition
// we will just omit putting the signatures into the header files
// when linker looks for this in interrupt_wrapper.S it will see these and be OK
int32_t sys_halt(uint8_t status) {
  printf("hi");
  return 0x20;
}
int32_t sys_execute(const uint8_t* command) {
  printf("hi");
  return 0x20;
}
int32_t sys_read(int32_t fd, void* buf, int32_t nbytes) {
  printf("hi");
  return 0x20;
}
int32_t sys_write(int32_t fd, const void* buf, int32_t nbytes) {
  dentry_t file;
  
}

int32_t sys_open(const uint8_t* filename) {
  // we need to look thru our filesystem for such a file
  dentry_t file;
  uint32_t ret = read_dentry_by_name(filename, &file);
  if (ret == -1) {
    // file not found
    return -1;
  }
  if (file.file_type == 1) {
    // directory
  }
  else if (file.file_type == 2) {
    // regular file
  }
  else if (file.file_type == 0) {
    // RTC

  } 
  else {
    // unknown
    return -1;
  }
}
int32_t sys_close(int32_t fd) {
  printf("hi");
  return 0x20;
}
int32_t sys_getargs(uint8_t* buf, int32_t nbytes) {
  printf("hi");
  return 0x20;
}
int32_t sys_vidmap(uint8_t** screen_start) {
  printf("hi");
  return 0x20;
}
int32_t sys_set_handler(int32_t signum, void* handler_address) {
  printf("hi");
  return 0x20;
}
int32_t sys_sigreturn(void) {
  printf("hi");
  return 0x20;
}
