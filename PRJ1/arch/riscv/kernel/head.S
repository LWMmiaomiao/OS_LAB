/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

#define KERNEL_STACK		0x50500000

.section ".entry_function","ax"
ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /* TODO: [p1-task2] clear BSS for flat non-ELF images */
  la a0, __bss_start
  la a1, __BSS_END__
  bge a0, a1, clear_done
clear_bss:
  sb zero, 0(a0)
  addi a0, a0, 1
  blt a0, a1, clear_bss

clear_done:

  /* TODO: [p1-task2] setup C environment */
  li sp, KERNEL_STACK // 创建栈空间为0x50500000
  li fp, KERNEL_STACK // fp
  j  main             // not jal 内核初始化只使用head.S一次


loop:
  wfi
  j loop

END(_start)
