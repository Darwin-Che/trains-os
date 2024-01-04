#include "lib/include/rpi.h"
#include "lib/include/printf.h"
#include "lib/include/uapi.h"
#include "lib/include/util.h"

void secondary_user_task();

void first_user_task()
{
  char msg[1024];

  int term_tid = ke_who_is(NAME_UART_TERM_SERVER);

  int my_tid = ke_my_tid();
  sprintf(msg, "PIE-C FirstUserTask started tid=%d (Priority 0)\r\n", my_tid);
  ke_puts(term_tid, TERM_CHANNEL, msg, strlen(msg));

  int child_tid;

  child_tid = ke_create(0, secondary_user_task);
  sprintf(msg, "PIE-C Created: %d (Priority 0)\r\n", child_tid);
  ke_puts(term_tid, TERM_CHANNEL, msg, strlen(msg));

  child_tid = ke_create(2, secondary_user_task);
  sprintf(msg, "PIE-C Created: %d (Priority 2)\r\n", child_tid);
  ke_puts(term_tid, TERM_CHANNEL, msg, strlen(msg));

  sprintf(msg, "PIE-C FirstUserTask: exiting\r\n");
  ke_puts(term_tid, TERM_CHANNEL, msg, strlen(msg));

  // ke_exit();
}

void secondary_user_task()
{
  int term_tid = ke_who_is(NAME_UART_TERM_SERVER);

  char msg[1024];
  int my_tid = ke_my_tid();
  int parent_tid = ke_my_parent_tid();

  sprintf(msg, "  PIE-C Second tid=%d parent_tid=%d\r\n", my_tid, parent_tid);
  ke_puts(term_tid, TERM_CHANNEL, msg, strlen(msg));

  ke_yield();

  sprintf(msg, "  PIE-C Second tid=%d parent_tid=%d Yield Finished\r\n", my_tid, parent_tid);
  ke_puts(term_tid, TERM_CHANNEL, msg, strlen(msg));

  // Testing ke_exit fail safe protection
  // ke_exit();
}

void _start()
{
  first_user_task();
}