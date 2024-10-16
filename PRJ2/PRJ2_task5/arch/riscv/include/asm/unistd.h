#ifndef __ASM_UNISTD_H__
#define __ASM_UNISTD_H__

#define SYSCALL_SLEEP 2
#define SYSCALL_YIELD 7
#define SYSCALL_WRITE 20
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23
#define SYSCALL_GET_TIMEBASE 30
#define SYSCALL_GET_TICK 31
#define SYSCALL_LOCK_INIT 40
#define SYSCALL_LOCK_ACQ 41
#define SYSCALL_LOCK_RELEASE 42

// for p2-task5
#define SYSCALL_WORKLOAD 50

#endif