#include "scheduler.h"
#include "util.h"
#include "debug_print.h"


void k_sched_init(struct kScheduler *sched)
{
  HEAP_INIT(&sched->task_heap, K_SCHED_HEAP_CAPACITY);
  sched->recv_blocked = NULL;
  sched->send_blocked = NULL;
  sched->reply_blocked = NULL;
  sched->event_blocked = NULL;
}

const char *k_sched_print(struct kScheduler *sched)
{
  static char str[] = "sched: ready - 00, recv_b - 00, send_b - 00, reply_b - 00, event_b - 00";

  struct kTaskDsp *it;
  size_t cnt_ready, cnt_recv, cnt_send, cnt_reply, cnt_event;
  cnt_ready = k_sched_len(sched);
  DL_COUNT2(sched->recv_blocked, it, cnt_recv, next_sched);
  DL_COUNT2(sched->send_blocked, it, cnt_send, next_sched);
  DL_COUNT2(sched->reply_blocked, it, cnt_reply, next_sched);
  DL_COUNT2(sched->event_blocked, it, cnt_event, next_sched);

  snprintf(str, sizeof(str), "sched: ready - %u, recv_b - %u, send_b - %u, reply_b - %u, event_b - %u", cnt_ready, cnt_recv, cnt_send, cnt_reply, cnt_event);
  return str;
}

size_t k_sched_len(struct kScheduler *sched)
{
  return sched->task_heap.count;
}
