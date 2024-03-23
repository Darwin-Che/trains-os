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

  // setup idle_task : tid = 0
  printf("Kernel Start - creating idle task\r\n");
  struct kTaskDsp *idle_td = k_tmgr_get_free_task(&gs.task_mgr, PG_SFT);
  if (idle_td == NULL)
  {
    printf("Failed to create idle_td\r\n");
    return 0;
  }
  k_td_init_user_task(idle_td, NULL, K_SCHED_PRIO_MIN, idle_task, NULL, 0);
  gs.task_mgr.idle_task = idle_td;

  // setup user_entry : tid = 1
  struct kTaskDsp *user_entry_td = k_tmgr_get_free_task(&gs.task_mgr, PG_SFT);
  if (user_entry_td == NULL)
  {
    printf("Failed to create user_entry_td\r\n");
    return 0;
  }
  printf("Kernel Start - creating user_entry - tid = %lld\r\n", user_entry_td->tid);
  const char user_entry_task_args[] = "PROGRAM\0user_entry";
  k_td_init_user_task(user_entry_td, NULL, K_SCHED_PRIO_MAX, load_elf, user_entry_task_args, sizeof(user_entry_task_args)); // init other fields
  k_sched_add_ready(&gs.scheduler, user_entry_td);

  // setup name_server : tid = 2
  struct kTaskDsp *name_server_td = k_tmgr_get_free_task(&gs.task_mgr, PG_SFT);
  if (name_server_td == NULL)
  {
    printf("Failed to create name_server\r\n");
    return 0;
  }
  printf("Kernel Start - creating name_server - tid = %lld\r\n", name_server_td->tid);
  const char name_server_task_args[] = "PROGRAM\0name_server";
  k_td_init_user_task(name_server_td, NULL, K_SCHED_PRIO_MAX, load_elf, name_server_task_args, sizeof(name_server_task_args)); // init other fields
  k_sched_add_ready(&gs.scheduler, name_server_td);

  // Start the Loop
  k_gic_enable();
  k_loop(&gs);

  return 0;
}
