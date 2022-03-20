#ifndef TASK_H
#define TASK_H

#include "filesystem.h"
#include "RTC.h"
#define MAX_OPEN_FILES 8

/*
The file operations jump table associated with the correct file type. 
This jump table should contain entries
for open, read, write, and close to perform type-specific actions 
for each operation. open is used for
performing type-specific initialization. For example, if we just open’d 
the RTC, the jump table pointer in this
structure should store the RTC’s file operations table.
*/
typedef struct jump_table_fd {
  int32_t (*read)(int32_t, void*, int32_t);
  int32_t (*write)(int32_t, const void*, int32_t);
  int32_t (*open)(const uint8_t*);
  int32_t (*close)(int32_t);
} jump_table_fd;

jump_table_fd rtc_jump_table = {
    read_RTC,
    write_RTC,
    open_RTC,
    close_RTC
};

typedef struct file_descriptor {
  jump_table_fd jump_table;
  uint32_t inode;
  uint32_t file_position;
  uint32_t flags;
} file_descriptor;

void init_task();
file_descriptor open_files[MAX_OPEN_FILES];

#endif