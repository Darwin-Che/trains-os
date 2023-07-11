#include "common/rpi.h"
#include "common/printf.h"
#include "kernel/uapi.h"

void secondary_user_task();

void first_user_task()
{
  char msg_start[] = "FirstUserTask: starting\r\n";
  uart_puts(0, 0, msg_start, sizeof(msg_start) - 1);

  char msg_create[] = "Created: 0\r\n";

  msg_create[9] = '0' + ke_create(0, secondary_user_task);

  uart_puts(0, 0, msg_create, sizeof(msg_create) - 1);

  msg_create[9] = '0' + ke_create(0, secondary_user_task);
  uart_puts(0, 0, msg_create, sizeof(msg_create) - 1);

  msg_create[9] = '0' + ke_create(2, secondary_user_task);
  uart_puts(0, 0, msg_create, sizeof(msg_create) - 1);

  msg_create[9] = '0' + ke_create(2, secondary_user_task);
  uart_puts(0, 0, msg_create, sizeof(msg_create) - 1);

  char msg_exit[] = "FirstUserTask: exiting\r\n";
  uart_puts(0, 0, msg_exit, sizeof(msg_exit) - 1);

  ke_exit();
}

void secondary_user_task()
{
  char msg1[] = "  Task ID : 0; ";
  char msg2[] = "Parent Task ID: 0 \r\n";

  msg1[12] = '0' + ke_my_tid();
  int parent_tid = ke_my_parent_tid();
  if (parent_tid == -1)
  {
    msg2[16] = '-';
    msg2[17] = '1';
  }
  else
  {
    msg2[16] = '0' + parent_tid;
    msg2[17] = ' ';
  }
  uart_puts(0, 0, msg1, sizeof(msg1) - 1);
  uart_puts(0, 0, msg2, sizeof(msg2) - 1);

  ke_yield();

  uart_puts(0, 0, msg1, sizeof(msg1) - 1);
  uart_puts(0, 0, msg2, sizeof(msg2) - 1);

  // Testing ke_exit fail safe protection
  // ke_exit();
}
