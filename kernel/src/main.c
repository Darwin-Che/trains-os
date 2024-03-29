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

  uart_init(0, 115200);

  uart_init(2, 115200);

  while (1) {
    spi_uart_putc(0, 0, 'X');
    uart_putc(2, 'Y');
    for (int i = 0; i < 65535; i += 1)
      asm volatile("nop");
  }

  exception_vector_setup();

  struct kGlobalState gs;
  k_gs_init(&gs);

#ifdef DEBUG
  k_page_table_print_all();
  uart_getc(0);
#endif

  printf("This is trains-os (" __TIME__ ")\r\n");
  uart_getc(0);

  printf("Enable MMU? y/n\r\n");
  char c = uart_getc(0);
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

  printf("Kernel Start - creating idle task\r\n");

  // setup idle_td
  struct kTaskDsp *idle_td = k_tmgr_get_free_task(&gs.task_mgr, PG_SFT);
  if (idle_td == NULL)
  {
    printf("Failed to create idle_td\r\n");
    return 0;
  }

  k_td_init_user_task(idle_td, NULL, K_SCHED_PRIO_MIN, idle_task, NULL, 0);
  gs.task_mgr.idle_task = idle_td;

  printf("Kernel Start - creating user_entry\r\n");

  struct kTaskDsp *td = k_tmgr_get_free_task(&gs.task_mgr, PG_SFT); // only tid is written
  if (td == NULL)
  {
    printf("Failed to create user_entry_td\r\n");
    return 0;
  }
  const char user_entry_task_args[] = "PROGRAM\0user_entry";
  k_td_init_user_task(td, NULL, K_SCHED_PRIO_MAX, load_elf, user_entry_task_args, sizeof(user_entry_task_args)); // init other fields
  k_sched_add_ready(&gs.scheduler, td);

  k_gic_enable();
  k_loop(&gs);

  return 0;
}
