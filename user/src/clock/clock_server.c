#include <stdint.h>
#include "lib/include/uapi.h"
#include "msg/msg.h"
#include "lib/include/assert.h"
#include "lib/include/timer.h"
#include "lib/include/dashboard.h"
#include "lib/include/heap.h"
#include "lib/include/utlist.h"

#define CK_SERVER_DELAY_QUEUE_SZ 256

struct WaitElem
{
  struct
  {
    int delay;
    int insert_val;
  } delay_until_tick;
  int tid;
  struct WaitElem *next_free;
};

#define heap_cmp_delay(v1, v2) ((v1).delay < (v2).delay)

HEAP_TYPE(WaitElem, CK_SERVER_DELAY_QUEUE_SZ, CkWaitHeap)
HEAPIFY_BOTTOM_TOP(CkWaitHeap, WaitElem, delay_until_tick, heap_cmp_delay)
HEAPIFY_TOP_BOTTOM(CkWaitHeap, WaitElem, delay_until_tick, heap_cmp_delay)

struct CkServer
{
  int curtick;
  struct WaitElem alloc_arr[CK_SERVER_DELAY_QUEUE_SZ];
  struct WaitElem *free_list;
  struct CkWaitHeap delay_queue;
};

#define TICK_IN_MMSEC (10 * 1000)
#define HALF_NOTIF (1 * 1000)

void clock_notifier_main();

void clock_server_init(struct CkServer *cs)
{
  cs->curtick = 0;

  util_memset(cs->alloc_arr, 0x0, sizeof(cs->alloc_arr));

  cs->free_list = NULL;
  for (int i = CK_SERVER_DELAY_QUEUE_SZ - 1; i >= 0; i -= 1)
    LL_PREPEND2(cs->free_list, &cs->alloc_arr[i], next_free);

  HEAP_INIT(&cs->delay_queue, CK_SERVER_DELAY_QUEUE_SZ);
}

bool clock_server_update_tick(struct CkServer *cs)
{
  cs->curtick += 1;
  return true;
}

void clock_server_pop_delay(struct CkServer *cs)
{
  struct WaitElem *next_delay;
  MSG_INIT(reply_time, mCkResp);
  reply_time.tick = cs->curtick;

  while (1)
  {
    HEAP_PEEK(&cs->delay_queue, next_delay);
    if ((next_delay == NULL) || (next_delay->delay_until_tick.delay > cs->curtick))
      break;
    HEAP_POP(CkWaitHeap, &cs->delay_queue, next_delay);
    LL_PREPEND2(cs->free_list, next_delay, next_free);

    MSG_REPLY(next_delay->tid, &reply_time);
  }
}

void clock_server_add_delay_until(struct CkServer *cs, int tid, int tick)
{
  struct WaitElem *elem = cs->free_list;
  ASSERT_MSG(elem != NULL, "Clock Server Delay Queue Full\r\n");
  LL_DELETE2(cs->free_list, elem, next_free);

  elem->delay_until_tick.delay = tick;
  elem->tid = tid;

  HEAP_INSERT(CkWaitHeap, &cs->delay_queue, delay_until_tick, elem);
}

void clock_server_main()
{
  struct CkServer cs;

  ke_create(KE_PRIO_MAX, clock_notifier_main);

  int tid;
  MSGBOX_T(recvbox, sizeof(struct mCkDelayReq));
  MSG_INIT(reply_notif, mCkNotifyResp);
  MSG_INIT(reply_time, mCkResp);

  clock_server_init(&cs);

  ke_register_as(NAME_CLOCK_SERVER);

  while (1)
  {
    MSG_RECV(&tid, &recvbox);
    if (MSGBOX_IS(&recvbox, mCkNotifyReq))
    {
      MSGBOX_CAST(&recvbox, mCkNotifyReq, _);
      MSG_REPLY(tid, &reply_notif);
      if (clock_server_update_tick(&cs))
        clock_server_pop_delay(&cs);
    }
    else if (MSGBOX_IS(&recvbox, mCkTimeReq))
    {
      MSGBOX_CAST(&recvbox, mCkTimeReq, _);
      reply_time.tick = cs.curtick;
      MSG_REPLY(tid, &reply_time);
    }
    else if (MSGBOX_IS(&recvbox, mCkDelayReq))
    {
      MSGBOX_CAST(&recvbox, mCkDelayReq, delayreq);
      if (delayreq->ticks < 0)
      {
        reply_time.tick = -2;
        MSG_REPLY(tid, &reply_time);
        continue;
      }
      clock_server_add_delay_until(&cs, tid, cs.curtick + delayreq->ticks);
    }
    else if (MSGBOX_IS(&recvbox, mCkDelayUntilReq))
    {
      MSGBOX_CAST(&recvbox, mCkDelayUntilReq, delayreq);
      if (delayreq->tick < cs.curtick)
      {
        reply_time.tick = -2;
        MSG_REPLY(tid, &reply_time);
        continue;
      }
      clock_server_add_delay_until(&cs, tid, delayreq->tick);
    }
    else
    {
      printf("clock_server receives invalid request\r\n");
      assert_fail();
    }
  }
}