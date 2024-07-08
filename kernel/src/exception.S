#include "sys_const.h"
.include "context_macro.S"

// 29 instruction -> Fit into exception vector
.macro syscall_handler
.align 7
// save all registers x19-x30
store_regs el0 // 7 instructions

// save pstate, sp, lr
store_td sysc // 7 instructions

// td->esr_el1 != 0 -> this is syscall 
mrs local0, esr_el1
str local0, [scratch0, #0x18]

extract_regs el1 // 7 instructions

ubfx local0, local0, #0, #25
lsl local0, local0, #0x2
adr local1, handler_func_tabl
add local0, local0, local1

br local0 // resume_kernel
b assert_fail // Never reach here
.align 7
.endm

// 35 instructions
.macro interrupt_handler
.align 7
// save all registers x1-x30
store_regs intr // 16 instructions

// save pstate, sp, pc
store_td intr // 8 instructions

// save register x0
str x0, [scratch0, #0x20]

// td->esr_el1 = 0 -> this is interrrupt
str xzr, [scratch0, #0x18]

// load kernel registers x19-x30
extract_regs el1 // 7 instructions

b k_intr_handler
b assert_fail // Never reach here
.align 7
.endm

.macro invalid_exception_handler
.align 7
mov x0, #2
bl print_debug
b assert_fail
.align 7
.endm

.global exception_vector_el1
.align 11
exception_vector_el1:
  invalid_exception_handler
  invalid_exception_handler
  invalid_exception_handler
  invalid_exception_handler

  invalid_exception_handler
  invalid_exception_handler
  invalid_exception_handler
  invalid_exception_handler
  
  syscall_handler // 29 instructions < 32
  interrupt_handler // 35 instructions > 32
  // invalid_exception_handler // overwritten by previous exception handler
  invalid_exception_handler
  
  invalid_exception_handler
  invalid_exception_handler
  invalid_exception_handler
  invalid_exception_handler

.align 2
.global exception_vector_setup
exception_vector_setup:
  adr x0, exception_vector_el1
  msr vbar_el1, x0
  ret

.global handler_func_tabl
handler_func_tabl:
  b k_exit_handler
  ret
  b k_create_handler
  b k_my_tid_handler
  b k_my_parent_tid_handler
  b k_send_handler
  b k_recv_handler
  b k_reply_handler
  b k_await_event_handler
  b k_mmap_handler
  b k_sys_health
  b k_quadrature_encoder_init
  b k_quadrature_encoder_get
  b k_print_raw
  b k_quit


