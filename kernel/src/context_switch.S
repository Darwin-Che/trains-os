.include "context_macro.S"
#include "sys_const.h"

.align 2

/*
 Perserve ABI
 Arguments:
 1. user_context
 */
.global kernel_into_user
kernel_into_user:
  // store x19-x30 for kernel
  store_regs el1

  // extract pstate, sp, pc
  extract_td

  // extract x19-x30 if syscall, x1-x30 if interrupt
  extract_regs el0

  // load the return value of the syscall
  ldr x0, [x0, #0x20]

  eret