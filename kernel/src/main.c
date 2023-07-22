#include "stack.h"
#include "task.h"
#include "global_state.h"
#include "loop.h"
#include "lib/include/util.h"
#include "lib/include/rpi.h"
#include "user/user_entry.h"
#include "interrupt.h"
#include "mmu.h"
#include "debug_print.h"

extern void exception_vector_setup();
extern void k_page_table_print_all();
extern void turn_on_mmu();

int k_main()
{
  init_gpio();
  init_spi(0);
  init_uart(0);

  exception_vector_setup();

  struct kGlobalState gs;
  k_gs_init(&gs);

#ifdef DEBUG
  k_page_table_print_all();
  uart_getc(0, 0);
#endif

  printf("This is trains-os (" __TIME__ ")\n");
  uart_getc(0, 0);

  printf("Enable MMU? y/n\r\n");
  char c = uart_getc(0, 0);
  if (c == 'y')
  {
    turn_on_mmu();
    printf("MMU FINISHED\r\n");
  }
  else
  {
    printf("MMU SKIPPED\r\n");
  }

  DEBUG_PRINT("Kernel Start\r\n");

  // setup idle_td
  struct kTaskDsp *idle_td = k_tmgr_get_free_task(&gs.task_mgr);
  k_td_init_user_task(idle_td, NULL, K_SCHED_PRIO_MIN, idle_task);
  gs.task_mgr.idle_task = idle_td;

  // Our first task
  struct kTaskDsp *td = k_tmgr_get_free_task(&gs.task_mgr);    // only tid is written
  k_td_init_user_task(td, NULL, K_SCHED_PRIO_MAX, user_entry); // init other fields
  k_sched_add_ready(&gs.scheduler, td);

  k_gic_enable();
  k_loop(&gs);

  return 0;
}
