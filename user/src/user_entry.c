#include "kernel/uapi.h"
#include "nameserver.h"
#include "rps.h"
#include "k2.h"
#include "k3.h"
#include "lib/include/printf.h"
#include "lib/include/rpi.h"
#include "lib/include/timer.h"
#include "clock.h"
#include "msg/msg.h"
#include "uart.h"

void user_entry()
{
  ke_create(KE_PRIO_MAX, ns_main);
  int clock_tid = ke_create(KE_PRIO_MAX, clock_server_main);
  (void)clock_tid;
  U_PRINT("CLOCK_TID : %d\r\n", clock_tid);

  int uart_term_tid = ke_create(KE_PRIO_UART_SERVER, uart_server_term_main);
  (void)uart_term_tid;
  U_PRINT("UART_TERM_TID : %d\r\n", uart_term_tid);
  // int uart_marklin_tid = ke_create(KE_PRIO_UART_SERVER, uart_server_marklin_main);
  // (void)uart_marklin_tid;
  // U_PRINT("UART_MARKLIN : %d\n", uart_marklin_tid);

  first_user_task();
}