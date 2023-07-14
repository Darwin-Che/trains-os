#ifndef K_SCHEDULER_H
#define K_SCHEDULER_H

#include "kernel/uapi.h"
#include "lib/include/heap.h"
#include "task.h"
#include <stdint.h>

#define K_SCHED_PRIO_MIN (KE_PRIO_MIN)
#define K_SCHED_PRIO_MAX (KE_PRIO_MAX)

#define K_SCHED_HEAP_CAPACITY 200

// setup heap for scheduler

HEAP_TYPE(kTaskDsp, K_SCHED_HEAP_CAPACITY, kTaskDspHeap)

static inline bool cmp_keys_task_dsp(struct kPriorityKey key1,
                                     struct kPriorityKey key2) {
  if (key1.priority == key2.priority) {
    return key1.insert_val < key2.insert_val;
  }
  return key1.priority < key2.priority;
}

HEAPIFY_BOTTOM_TOP(kTaskDspHeap, kTaskDsp, priority_key, cmp_keys_task_dsp)
HEAPIFY_TOP_BOTTOM(kTaskDspHeap, kTaskDsp, priority_key, cmp_keys_task_dsp)

struct kScheduler {
  struct kTaskDspHeap task_heap;
  struct kTaskDsp *recv_blocked;
  struct kTaskDsp *send_blocked;
  struct kTaskDsp *reply_blocked;
  struct kTaskDsp *event_blocked;
};

void k_sched_init(struct kScheduler *sched);
const char *k_sched_print(struct kScheduler *sched);
size_t k_sched_len(struct kScheduler *sched);

static inline bool k_sched_priority_is_valid(int priority) {
  return priority >= K_SCHED_PRIO_MAX && priority <= K_SCHED_PRIO_MIN;
}

static inline struct kTaskDsp *k_sched_get_next(struct kScheduler *sched) {
  struct kTaskDsp *task_dsp_ptr;
  HEAP_POP(kTaskDspHeap, &sched->task_heap, task_dsp_ptr);
  return task_dsp_ptr;
}

static inline void k_sched_add_ready(struct kScheduler *sched,
                                     struct kTaskDsp *td) {
  td->state = READY;
  HEAP_INSERT(kTaskDspHeap, &sched->task_heap, priority_key, td);
}

static inline void k_sched_add_reply_wait(struct kScheduler *sched,
                                          struct kTaskDsp *td) {
  DL_APPEND2(sched->reply_blocked, td, prev_sched, next_sched);
  td->state = REPLY_BLOCKED;
}

static inline void k_sched_remove_reply_wait(struct kScheduler *sched,
                                             struct kTaskDsp *td) {
  assert(td->state == REPLY_BLOCKED);
  DL_DELETE2(sched->reply_blocked, td, prev_sched, next_sched);
}

static inline void k_sched_add_send_wait(struct kScheduler *sched,
                                         struct kTaskDsp *td) {
  DL_APPEND2(sched->send_blocked, td, prev_sched, next_sched);
  td->state = SEND_BLOCKED;
}

static inline void k_sched_remove_send_wait(struct kScheduler *sched,
                                            struct kTaskDsp *td) {
  assert(td->state == SEND_BLOCKED);
  DL_DELETE2(sched->send_blocked, td, prev_sched, next_sched);
}

static inline void k_sched_add_recv_wait(struct kScheduler *sched,
                                         struct kTaskDsp *td) {
  DL_APPEND2(sched->recv_blocked, td, prev_sched, next_sched);
  td->state = RECEIVE_BLOCKED;
}

static inline void k_sched_remove_recv_wait(struct kScheduler *sched,
                                            struct kTaskDsp *td) {
  assert(td->state == RECEIVE_BLOCKED);
  DL_DELETE2(sched->recv_blocked, td, prev_sched, next_sched);
}

static inline void k_sched_add_event_wait(struct kScheduler *sched,
                                          struct kTaskDsp *td,
                                          enum keIntrId intr_id) {
  DL_APPEND2(sched->event_blocked, td, prev_sched, next_sched);
  td->state = (enum kTaskState)intr_id;
}

static inline void k_sched_remove_event_wait(struct kScheduler *sched,
                                             struct kTaskDsp *td) {
  assert(IS_EVENT_BLOCKED(td->state));
  DL_DELETE2(sched->event_blocked, td, prev_sched, next_sched);
}

#endif
