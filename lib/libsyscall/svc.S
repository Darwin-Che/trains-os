.macro handler_branch api_func syscallno
.global \api_func
\api_func:
  svc #\syscallno
  b assert_fail // Never reach here
.endm

handler_branch ke_exit 0
handler_branch ke_yield 1
handler_branch ke_create 2
handler_branch ke_my_tid 3
handler_branch ke_my_parent_tid 4
handler_branch ke_send 5
handler_branch ke_recv 6
handler_branch ke_reply 7
handler_branch ke_await_event 8
handler_branch ke_mmap 9
handler_branch ke_sys_health 10
handler_branch ke_uart_write_reg 11
handler_branch ke_uart_read_reg 12
handler_branch ke_quit 13