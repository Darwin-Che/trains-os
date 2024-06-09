#ifndef K_TASK_H
#define K_TASK_H

#include "hashtable.h"
#include "lib/include/rbtree.h"
#include "lib/include/uapi.h"
#include "lib/include/utlist.h"
#include "mailbox.h"
#include <stddef.h>
#include <stdint.h>

#define kg_current_td (*(struct kTaskDsp **)CUR_ACTIVE_TD_PTR)

#define K_TID_INVALID -1

struct kRegStore
{
  uint64_t x29;
  uint64_t x30;
  uint64_t x27;
  uint64_t x28;
  uint64_t x25;
  uint64_t x26;
  uint64_t x23;
  uint64_t x24;
  uint64_t x21;
  uint64_t x22;
  uint64_t x19;
  uint64_t x20;
  uint64_t x17;
  uint64_t x18;
  uint64_t x15;
  uint64_t x16;
  uint64_t x13;
  uint64_t x14;
  uint64_t x11;
  uint64_t x12;
  uint64_t x09;
  uint64_t x10;
  uint64_t x07;
  uint64_t x08;
  uint64_t x05;
  uint64_t x06;
  uint64_t x03;
  uint64_t x04;
  uint64_t x01;
  uint64_t x02;
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
  uint64_t
      esr_el1; // 0 if interrupt, otherwise take bits [0, 24] for the syscall_no
  uint64_t syscall_retval;

  // int64_t stack_id;
  uintptr_t stack_addr;
  uint32_t stack_sz; // The count of bytes
  int64_t tid;       // tid should only be negative after process is destroyed
  struct kPriorityKey priority_key;
  uint32_t state; // enum kTaskState

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

  struct rb_node rb_link_tid;

  struct kMailbox mailbox;
};

struct kTaskDspMgr
{
  struct SlabAlloc *task_alloc;
  struct kTaskDsp *idle_task;
  int64_t next_tid;
  struct rb_root map_tid;
};

void k_tmgr_init(struct kTaskDspMgr *mgr);
struct kTaskDsp *k_tmgr_get_free_task(struct kTaskDspMgr *gs, uint8_t sz);
void k_tmgr_destroy_task(struct kTaskDspMgr *mg, struct kTaskDsp *td);

void k_td_print(struct kTaskDsp *kd);
int k_td_get_syscall_no(struct kTaskDsp *kd);

void k_td_init_user_task(struct kTaskDsp *td, struct kTaskDsp *parent_td, uint64_t priority,
                         void (*user_func)(),
                         const char *args, size_t args_len);

static inline int k_td_rb_cmp_tid_key(const void *key,
                                      const struct rb_node *node)
{
  int64_t diff =
      *(const int64_t *)key - rb_entry(node, struct kTaskDsp, rb_link_tid)->tid;
  if (diff == 0)
    return 0;
  else if (diff < 0)
    return -1;
  else
    return 1;
}

static inline int k_td_rb_cmp_tid(struct rb_node *node1,
                                  const struct rb_node *node2)
{
  int64_t diff = rb_entry(node1, struct kTaskDsp, rb_link_tid)->tid -
                 rb_entry(node2, struct kTaskDsp, rb_link_tid)->tid;
  if (diff == 0)
    return 0;
  else if (diff < 0)
    return -1;
  else
    return 1;
}

static inline struct kTaskDsp *k_task_mgr_get_task_dsp(struct kTaskDspMgr *mgr,
                                                       int64_t tid)
{
  struct rb_node *node = rb_find(&tid, &mgr->map_tid, k_td_rb_cmp_tid_key);
  if (node == NULL)
    return NULL;
  return rb_entry(node, struct kTaskDsp, rb_link_tid);
}

#endif
