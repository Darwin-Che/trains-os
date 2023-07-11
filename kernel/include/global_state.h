#ifndef K_GLOBAL_STATE_H
#define K_GLOBAL_STATE_H

#include "scheduler.h"
#include "task.h"

struct kGlobalState
{
  struct kTaskDspMgr task_mgr;
  struct kScheduler scheduler;

  uint64_t idle_track_start_time;
  uint64_t idle_duration;
  uint64_t next_intr_time;
};

extern struct kGlobalState *kg_gs;

void k_gs_init(struct kGlobalState *gs);

#endif
