#ifndef K_TASK_H
#define K_TASK_H

#include "hashtable.h"
#include "lib/include/utlist.h"
#include "mailbox.h"
#include "stack.h"
#include "kernel/uapi.h"
#include <stddef.h>
#include <stdint.h>

#define kg_current_td (*(struct kTaskDsp **)CUR_ACTIVE_TD_PTR)

#define K_TID_INVALID -1

struct kRegStore
{
  uint64_t regs[12];
};

enum kTaskState
{
  ACTIVE,
  READY,
  ZOMBIE,
  SEND_BLOCKED,
  RECEIVE_BLOCKED,
  REPLY_BLOCKED,
  EVENT_BLOCKED_RANGE = KE_INTR_ID_RANGE_MIN,
};

#define IS_EVENT_BLOCKED(s) (s >= EVENT_BLOCKED_RANGE)

struct kPriorityKey
{
  uint64_t priority; // only positive priority -  0 = max priority
  uint64_t insert_val;
};

struct kTaskDsp
{
  // written by `store_regs`
  uint64_t pc;
  uint64_t sp;
  uint64_t pstate; // only make sense in user tasks

  // offset 0x18

  // written by `syscall_handler`
  uint64_t esr_el1; // 0 if interrupt, otherwise take bits [0, 24] for the syscall_no
  uint64_t syscall_retval;

  int64_t stack_id;
  int64_t tid; // tid should only be negative after process is destroyed
  struct kPriorityKey priority_key;
  enum kTaskState state;

  int64_t parent_tid;

  union
  {
    struct kTaskDsp *next_free;
    struct kTaskDsp *next_zombie;
    struct
    {
      struct kTaskDsp *next_sched;
      struct kTaskDsp *prev_sched;
    };
    // struct kTaskDsp *next_send_task;
  };

  struct kMailbox mailbox;
};

struct kTaskDspMgr
{
  struct kTaskDsp task_arr[USER_STACK_ARRAY_CNT];
  struct kTaskDsp *free_list_head;
  struct kTaskDsp *idle_task;
  int64_t next_tid;
  struct kHtTask map_tid_to_td;
};

void k_tmgr_init(struct kTaskDspMgr *mgr);
struct kTaskDsp *k_tmgr_get_free_task(struct kTaskDspMgr *gs);
void k_tmgr_destroy_task(struct kTaskDspMgr *mg, struct kTaskDsp *td);

void k_td_print(struct kTaskDsp *kd);
int k_td_get_syscall_no(struct kTaskDsp *kd);

void k_td_init_user_task(struct kTaskDsp *td, struct kTaskDsp *parent_td,
                         uint64_t priority, void (*user_func)());

static inline struct kTaskDsp *k_task_mgr_get_task_dsp(struct kTaskDspMgr *mgr, int64_t tid)
{
  struct kHtTaskElem *elem;
  HT_FIND(&mgr->map_tid_to_td, &tid, elem);
  if (elem == NULL)
    return NULL;
  return elem->val;
}

#endif
