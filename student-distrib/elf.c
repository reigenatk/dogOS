#include "elf.h"
#include "task.h"
#include "system_calls.h"
#include "errno.h"

int elf_load(int fd) {

}

int elf_sanity(int fd) {
  elf_eheader_t eh;
  int ret = sys_read(fd, &eh, sizeof(elf_eheader_t));
  if (ret < 0) {
    return ret;
  }
  if (ret != sizeof(eh)) {
		return -EIO;
	}
	// Sanity checks
	if (eh.magic[0] != '\x7f' || eh.magic[1] != 'E' ||
		eh.magic[2] != 'L' || eh.magic[3] != 'F') {
		return -ENOEXEC;
	}
	
	if (eh.arch != 1) {
		// Not 32 bits
		return -ENOEXEC;
	}
	if (eh.endianness != 1) {
		// Not LE
		return -ENOEXEC;
	}
	if (eh.machine != 3) {
		// Not x86
		return -ENOEXEC;
	}
	
	return 0;
}

