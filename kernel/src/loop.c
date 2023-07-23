#include "loop.h"
#include "lib/include/timer.h"
#include "lib/include/rpi.h"
#include "kernel/uapi.h"
#include "task.h"
#include "lib/include/util.h"
#include "util.h"
#include "user/user_entry.h"
#include "debug_print.h"

extern uint64_t kernel_into_user();

void k_loop(struct kGlobalState *gs)
{
  uint64_t idle_ts = get_current_time_u64();

  while (1)
  {
    DEBUG_PRINT("%s\r\n", k_sched_print(&gs->scheduler));

    struct kTaskDsp *td = k_sched_get_next(&gs->scheduler);

    if (td != NULL)
    {
      DEBUG_PRINT("Scheduled tid %u\r\n", td->tid);
    }
    else
    {
      DEBUG_PRINT("Scheduled idle task\r\n");
      td = gs->task_mgr.idle_task;
      idle_ts = get_current_time_u64();
    }

    kg_current_td = td; // set to current active task

    kernel_into_user();

    DEBUG_PRINT("Syscall number %d\r\n", k_td_get_syscall_no(td));

    if (kg_current_td == gs->task_mgr.idle_task)
    {
      // Book keep the idle duration
      gs->idle_duration += (get_current_time_u64() - idle_ts);
    }
    else if (kg_current_td != NULL)
    {
      // Exiting from task sets kg_current_td = NULL so this will not add
      k_sched_add_ready(&gs->scheduler, kg_current_td);
    }
  }
}
