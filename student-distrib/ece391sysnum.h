#if !defined(ECE391SYSNUM_H)
#define ECE391SYSNUM_H

#define SYS_HALT    1
#define SYS_EXECUTE 2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_GETARGS 7
#define SYS_VIDMAP  8
#define SYS_SET_HANDLER  9
#define SYS_SIGRETURN  10

#endif /* ECE391SYSNUM_H */

// https://thevivekpandey.github.io/posts/2017-09-25-linux-system-calls.html
// slightly out of order but again the numbers dont matter so much as how many we have
#define SYSCALL_OPEN		16
#define SYSCALL_CLOSE		17
#define SYSCALL_READ		18
#define SYSCALL_WRITE		19
#define SYSCALL_MOUNT		20
#define SYSCALL_UMOUNT		21
#define SYSCALL_GETDENTS	22

#define SYSCALL_FORK		23
#define SYSCALL__EXIT		24
#define SYSCALL_EXECVE		25
#define SYSCALL_SIGACTION	26
#define SYSCALL_KILL		27
#define SYSCALL_SIGSUSPEND	28
#define SYSCALL_SIGPROCMASK	29
#define SYSCALL_WAITPID		30
#define SYSCALL_STAT		31
#define SYSCALL_FSTAT		32
#define SYSCALL_LSTAT		33
#define SYSCALL_GETPID		34
#define SYSCALL_BRK			35
#define SYSCALL_SBRK		36

#define SYSCALL_LSEEK		35
#define SYSCALL_CHMOD		36
#define SYSCALL_CHOWN		37
#define SYSCALL_LINK		38
#define SYSCALL_UNLINK		39
#define SYSCALL_SYMLINK		40
#define SYSCALL_READLINK	41
#define SYSCALL_TRUNCATE	42
#define SYSCALL_RENAME		44
#define SYSCALL_GETCWD		45
#define SYSCALL_CHDIR		46
#define SYSCALL_MKDIR		47
#define SYSCALL_RMDIR		48
#define SYSCALL_IOCTL		49

#define SYSCALL_GETUID		51
#define SYSCALL_SETUID		52
#define SYSCALL_GETGID		53
#define SYSCALL_SETGID		54