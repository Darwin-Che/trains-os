#include "util.h"
#include "global_state.h"
#include "common/timer.h"

struct kGlobalState *kg_gs;

void k_gs_init(struct kGlobalState *gs)
{
  k_tmgr_init(&gs->task_mgr);
  k_sched_init(&gs->scheduler);
  kg_gs = gs;

  gs->idle_track_start_time = get_current_time_u64();
  gs->idle_duration = 0;

  gs->next_intr_time = 0; // Initialized in gic_enable()
}
