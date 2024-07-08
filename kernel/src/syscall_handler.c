#include "task.h"
#include "global_state.h"
#include "scheduler.h"
#include "util.h"
#include "scheduler.h"
#include "mailbox.h"
#include "lib/include/rpi.h"
#include "debug_print.h"
#include "lib/include/uapi.h"
#include "lib/include/timer.h"
#include "lib/include/util.h"
#include "interrupt.h"
#include "pgmgr.h"
#include "lib/include/macro.h"
#include "sys_val.h"
#include "loader/include/loader.h"

void k_create_handler(int priority, const char * args, size_t args_len)
{
  DEBUG_PRINT("\r\n");
  // printf("k_create_handler %s %d\r\n", args, args_len);
  struct kTaskDsp *parent_td = kg_current_td;
  struct kTaskDsp *td = k_tmgr_get_free_task(&kg_gs->task_mgr, PG_SFT);
  if (td == NULL)
  {
    kg_current_td->syscall_retval = -2;
    return;
  }
  if (!k_sched_priority_is_valid(priority))
  {
    DEBUG_PRINT("Invalid priority received\r\n");
    kg_current_td->syscall_retval = -1;
    return;
  }
  k_td_init_user_task(td, parent_td, priority, load_elf, args, args_len);
  k_sched_add_ready(&kg_gs->scheduler, td);
  kg_current_td->syscall_retval = td->tid;
}

void k_exit_handler()
{
  k_tmgr_destroy_task(&kg_gs->task_mgr, kg_current_td);
  kg_current_td = NULL;
}

void k_my_tid_handler()
{
  DEBUG_PRINT("\r\n");
  assert(kg_current_td != NULL);
  kg_current_td->syscall_retval = kg_current_td->tid;
}

void k_my_parent_tid_handler()
{
  DEBUG_PRINT("\r\n");
  assert(kg_current_td != NULL);
  if (kg_current_td->parent_tid != K_TID_INVALID)
  {
    struct kTaskDsp *parent_td = k_task_mgr_get_task_dsp(&kg_gs->task_mgr, kg_current_td->parent_tid);
    if (parent_td == NULL)
    {
      kg_current_td->parent_tid = K_TID_INVALID;
    }
  }
  kg_current_td->syscall_retval = kg_current_td->parent_tid;
}

void k_send_handler(int tid, const char *msg, size_t msglen, char *reply, size_t rplen)
{
  DEBUG_PRINT("\r\n");
  struct kTaskDsp *sender_td = kg_current_td;
  assert(sender_td != NULL);
  struct kTaskDsp *receiver_td = k_task_mgr_get_task_dsp(&kg_gs->task_mgr, tid);
  if (receiver_td == NULL)
  {
    // DEBUG_PRINT("The receiver tid is invalid\r\n");
    kg_current_td->syscall_retval = (uint64_t)-1;
    return;
  }

  // setup mailbox
  k_mbox_send_init(&sender_td->mailbox, msg, msglen, reply, rplen);

  if (receiver_td->state == RECEIVE_BLOCKED)
  {
    // DEBUG_PRINT("receiver %d\r\n", receiver_td->tid);
    k_mbox_copy_msg(&receiver_td->mailbox, &sender_td->mailbox);
    *receiver_td->mailbox.in_tid = sender_td->tid;
    receiver_td->syscall_retval = sender_td->mailbox.out_msg_len;

    k_sched_add_reply_wait(&kg_gs->scheduler, sender_td);
    k_sched_remove_recv_wait(&kg_gs->scheduler, receiver_td);
    k_sched_add_ready(&kg_gs->scheduler, receiver_td);
  }
  else
  {
    k_mbox_add_send_queue(&receiver_td->mailbox, sender_td);

    k_sched_add_send_wait(&kg_gs->scheduler, sender_td);
  }

  kg_current_td = NULL;
}

void k_recv_handler(int *tid, char *msg, size_t msglen)
{
  DEBUG_PRINT("\r\n");
  struct kTaskDsp *receiver_td = kg_current_td;
  assert(receiver_td != NULL);
  struct kTaskDsp *sender_td = k_mbox_pop_send_queue(&receiver_td->mailbox);

  k_mbox_recv_init(&receiver_td->mailbox, msg, msglen, tid);

  if (sender_td == NULL)
  {
    k_sched_add_recv_wait(&kg_gs->scheduler, receiver_td);
  }
  else
  {
    // DEBUG_PRINT("sender %d\r\n", sender_td->tid);
    k_mbox_copy_msg(&receiver_td->mailbox, &sender_td->mailbox);
    *receiver_td->mailbox.in_tid = sender_td->tid;
    receiver_td->syscall_retval = sender_td->mailbox.out_msg_len;

    k_sched_remove_send_wait(&kg_gs->scheduler, sender_td);
    k_sched_add_reply_wait(&kg_gs->scheduler, sender_td);
    k_sched_add_ready(&kg_gs->scheduler, receiver_td);
  }

  kg_current_td = NULL;
}

void k_reply_handler(int tid, const char *reply, size_t rplen)
{
  DEBUG_PRINT("\r\n");
  struct kTaskDsp *receiver_td = kg_current_td;
  assert(receiver_td != NULL);
  struct kTaskDsp *sender_td = k_task_mgr_get_task_dsp(&kg_gs->task_mgr, tid);
  if (sender_td == NULL)
  {
    receiver_td->syscall_retval = -1;
  }
  else if (sender_td->state != REPLY_BLOCKED)
  {
    receiver_td->syscall_retval = -2;
  }
  else
  {
    sender_td->syscall_retval = rplen;
    receiver_td->syscall_retval = MIN(rplen, sender_td->mailbox.in_buf_len);
    util_memcpy(sender_td->mailbox.in_buf, reply, receiver_td->syscall_retval);

    k_sched_remove_reply_wait(&kg_gs->scheduler, sender_td);
    k_sched_add_ready(&kg_gs->scheduler, sender_td);
  }
}

void k_await_event_handler(uint32_t intr_id)
{
  DEBUG_PRINT("intr_id = %x\r\n", intr_id);
  // uint32_t gic_id = INTR_ID_GIC(intr_id);
  // if (gic_id == 97)
  // {
  //   k_intr_timer_arm();
  // }
  // else if (gic_id == 153)
  // {
  //   k_intr_uart_arm(INTR_ID_SUB(intr_id));
  // }
  // else
  // {
  //   kg_current_td->syscall_retval = -1;
  //   return;
  // }

  k_sched_add_event_wait(&kg_gs->scheduler, kg_current_td, intr_id);
  kg_current_td = NULL;
}

void k_mmap_handler(void **target, uint64_t mem_sz)
{
  mem_sz = NEXT_POW2(mem_sz);
  DEBUG_PRINT("k_mmap mem_sz = %x\r\n", mem_sz);
  uint8_t mem_sz_sft = MSB_POS(mem_sz);
  DEBUG_PRINT("k_mmap mem_sz_sft = %x\r\n", mem_sz_sft);
  *target = pg_alloc_page(SYSADDR.pgmgr, mem_sz_sft, 0);
  kg_current_td->syscall_retval = 0;
}

void k_sys_health(struct keSysHealth *health)
{
  DEBUG_PRINT("\r\n");
  uint64_t curtime = get_current_time_u64();
  // idle_percent is returned as ABCD to be output as AB.CD%
  health->idle_percent = (kg_gs->idle_duration * 100 * 100) / (curtime - kg_gs->idle_track_start_time);
  // Reset health check variables
  kg_gs->idle_duration = 0;
  kg_gs->idle_track_start_time = curtime;
  kg_current_td->syscall_retval = 0;
}

void k_quadrature_encoder_init(uint32_t pin_a, uint32_t pin_b) {
  DEBUG_PRINT("\r\n");
  int id = encoder_init(&kg_gs->encoder_mgr, pin_a, pin_b);
  kg_current_td->syscall_retval = id;
}

void k_quadrature_encoder_get(int id, struct keQuadEncoderStat * stat) {
  DEBUG_PRINT("\r\n");
  struct kQuadEncoder * encoder = get_encoder(&kg_gs->encoder_mgr, id);
  if (encoder == NULL) {
    kg_current_td->syscall_retval = 0;
  } else {
    kg_current_td->syscall_retval = 0;
    *stat = encoder->stat;
    ke_quad_encoder_stat_clear(&encoder->stat);
  }
}

void k_print_raw(const char * msg, int len)
{
  // printf("ke_print_raw %p %d\r\n", msg, len);
  if (msg != NULL) {
    for (int i = 0; i < len; i += 1)
      uart_putc(2, msg[i]);
    // for (int i = 0; i < len; i += 1)
    //   printf("msg[%d] = %d\r\n", i, (int) msg[i]);
  }
  else
    printf("NULL\r\n");

  for (int i = 0; i <= 65535; i += 1)
    asm volatile("yield");
  kg_current_td->syscall_retval = 0;
}

extern void el1_reboot();
void k_quit()
{
  printf(TERM_CLEAR);
  printf("BYE-BYE\r\n");
  el1_reboot();
}