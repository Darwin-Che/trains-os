#include "task.h"
#include "global_state.h"
#include "loop.h"
#include "lib/include/util.h"
#include "lib/include/rpi.h"
#include "user/user_entry.h"
#include "interrupt.h"
#include "mmu.h"
#include "debug_print.h"
#include "pgmgr.h"
#include "sys_val.h"
#include "bootstrap.h"
#include "loader/include/loader.h"

extern void exception_vector_setup();
extern void k_page_table_print_all();
extern void turn_on_mmu();

void idle_task()
{
  while (1)
  {
    asm volatile("wfi");
  }
}

int k_main()
{
  k_bootstrap_slab();

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

  printf("This is trains-os (" __TIME__ ")\r\n");
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

  pgmgr_debug_print(SYSADDR.pgmgr, false);

  printf("Kernel Start\r\n");

  // setup idle_td
  struct kTaskDsp *idle_td = k_tmgr_get_free_task(&gs.task_mgr, PG_SFT);
  if (idle_td == NULL)
  {
    printf("Failed to create idle_td\r\n");
    return 0;
  }

  k_td_init_user_task(idle_td, NULL, K_SCHED_PRIO_MIN, idle_task, 0, NULL, 0);
  gs.task_mgr.idle_task = idle_td;

  // Our first task
  struct kTaskDsp *td = k_tmgr_get_free_task(&gs.task_mgr, PG_SFT); // only tid is written
  if (td == NULL)
  {
    printf("Failed to create user_entry_td\r\n");
    return 0;
  }
  k_td_init_user_task(td, NULL, K_SCHED_PRIO_MAX, load_elf, 0x4000000, NULL, 0); // init other fields
  k_sched_add_ready(&gs.scheduler, td);

  k_gic_enable();
  k_loop(&gs);

  return 0;
}
