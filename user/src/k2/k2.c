#include "k2.h"
#include "kernel/uapi.h"
#include "lib/include/printf.h"
#include "lib/include/timer.h"

#define FIX_CHAR 0x5A
#define WRONG_CHAR 0x68

void k2_send();
void k2_recv();

bool quit;
int retval;
bool send_first;
int global_sz;
char msg_send_arr[512] __attribute__((aligned(16)));
char msg_recv_arr[512] __attribute__((aligned(16)));
char msg_reply_send_arr[512] __attribute__((aligned(16)));
char msg_reply_recv_arr[512] __attribute__((aligned(16)));
char *msg_send = msg_send_arr;
char *msg_recv = msg_recv_arr;
char *msg_reply_send = msg_reply_send_arr;
char *msg_reply_recv = msg_reply_recv_arr;
// char *msg_send = msg_send_arr + 1;
// char *msg_recv = msg_recv_arr + 1;
// char *msg_reply_send = msg_reply_send_arr + 1;
// char *msg_reply_recv = msg_reply_recv_arr + 1;

uint64_t cnt = 0;
uint64_t s;
uint64_t c;
const uint64_t dur = 10 * 1000 * 1000; // 10s
// const uint64_t dur = 0; // 10s
const uint64_t check_cnt = 100000;
// const uint64_t check_cnt = 1;

static inline void timer_starts()
{
  // printf("Timer starts\r\n");
  s = get_current_time_u64();
  cnt = 0;
  quit = false;
}

static inline bool check_exit()
{
  c = get_current_time_u64();
  return c - s >= dur;
}

static inline void inc()
{
  cnt += 1;
}

static inline void timer_output()
{
  printf("\r\n");
  printf("-----Test Result-----\r\n");
  printf("     SenderFirst=%d\r\n", send_first);
  printf("     SIZE=%d\r\n", global_sz);
  printf("     LOOP=%llu\r\n", cnt);
  printf("     MMSEC=%llu\r\n", c - s);
  printf("     AVG=%d\r\n", (c - s) / cnt);
  printf("\r\n");
}

void k2_entry()
{
  int sz_arr[] = {4, 64, 256};

  for (int i = 0; i < 256; i += 1)
  {
    msg_send[i] = FIX_CHAR;
    msg_recv[i] = WRONG_CHAR;
    msg_reply_send[i] = FIX_CHAR;
    msg_reply_recv[i] = WRONG_CHAR;
  }

  ke_register_as("k2_entry");

  for (int i = 0; i < 3; i += 1)
  {
    for (int j = 0; j < 2; j += 1)
    {
      send_first = (j == 0);
      global_sz = sz_arr[i];

      if (send_first)
      {
        ke_create(0, k2_recv);
        ke_create(1, k2_send);
      }
      else
      {
        ke_create(1, k2_recv);
        ke_create(0, k2_send);
      }

      int tid1, tid2;

      // printf("k2_entry: Init Experiment\r\n");
      retval = ke_recv(&tid1, NULL, 0); // Give the recv/send chance to initialize
      retval = ke_recv(&tid2, NULL, 0); // Give the recv/send chance to initialize

      printf("k2_entry: Start Experiment %d %d\r\n", tid1, tid2);
      // start the experiment
      retval = ke_reply(tid1, NULL, 0);
      retval = ke_reply(tid2, NULL, 0);

      // wake up when experiment ends
      retval = ke_recv(&tid1, NULL, 0);
      retval = ke_recv(&tid2, NULL, 0);
      printf("k2_entry: Finish Experiment\r\n");

      // let the children exit
      retval = ke_reply(tid1, NULL, 0);
      retval = ke_reply(tid2, NULL, 0);
      // printf("k2_entry: Exit Experiment\r\n");
    }
  }

  printf("k2_entry: Exit\r\n");
}

void k2_recv()
{
  int my_tid = ke_my_tid();
  printf("k2_recv %d: Start\r\n", my_tid);

  ke_register_as("k2_recv");

  int parent_tid = ke_who_is("k2_entry");

  // let the parent know we are ready
  retval = ke_send(parent_tid, NULL, 0, NULL, 0);

  if (!send_first)
    timer_starts();

  int tid;
  while (1)
  {
    retval = ke_recv(&tid, msg_recv, global_sz);
    retval = ke_reply(tid, msg_reply_send, global_sz);

    if (!send_first)
    {
      inc();
      if (cnt % check_cnt == 0 && check_exit())
      {
        quit = true;
        break;
      }
    }
    else if (quit)
      break;
  }

  for (int i = 0; i < global_sz; i += 1)
  {
    if (msg_recv[i] != FIX_CHAR)
    {
      printf("Test Fails: RECV not correct message!\r\n");
      printf("RECV : %X", msg_recv[i]);
      break;
    }
  }

  if (!send_first)
    timer_output();

  retval = ke_send(parent_tid, NULL, 0, NULL, 0);

  printf("k2_recv %d: Exit\r\n", my_tid);
}

void k2_send()
{
  int my_tid = ke_my_tid();
  printf("k2_send %d: Start\r\n", my_tid);

  int parent_tid = ke_who_is("k2_entry");

  // let the parent know we are ready
  retval = ke_send(parent_tid, NULL, 0, NULL, 0);

  // Only query recv_tid after setup are finished
  int recv_tid = ke_who_is("k2_recv");

  if (send_first)
    timer_starts();

  while (1)
  {
    retval = ke_send(recv_tid, msg_send, global_sz, msg_reply_recv, global_sz);

    if (send_first)
    {
      inc();
      if (cnt % check_cnt == 0 && check_exit())
      {
        quit = true;
        break;
      }
    }
    else if (quit)
      break;
  }

  for (int i = 0; i < global_sz; i += 1)
  {
    if (msg_reply_recv[i] != FIX_CHAR)
    {
      printf("Test Fails: REPLY not correct message!\r\n");
      printf("REPLY : %X", msg_reply_recv[i]);
      break;
    }
  }

  if (send_first)
    timer_output();

  retval = ke_send(parent_tid, NULL, 0, NULL, 0);

  printf("k2_send %d: Exit\r\n", my_tid);
}