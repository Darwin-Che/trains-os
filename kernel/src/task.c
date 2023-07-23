#include "task.h"
#include "lib/include/rpi.h"
#include "util.h"
#include "lib/include/util.h"
#include "global_state.h"
#include "debug_print.h"
#include "kernel/uapi.h"
#include "lib/include/hashtable.h"
#include "pgmgr.h"
#include "sys_val.h"

void k_tmgr_init(struct kTaskDspMgr *mgr)
{
  mgr->free_list_head = NULL;
  // 0 -> 1 -> ... -> n-1
  for (int i = USER_STACK_ARRAY_CNT - 1; i >= 0; i -= 1)
  {
    mgr->task_arr[i].tid = K_TID_INVALID;
    LL_PREPEND2(mgr->free_list_head, &mgr->task_arr[i], next_free);
  }

  HT_INIT(&mgr->map_tid_to_td, K_TASK_HT_BUCKSZ, K_TASK_HT_CAPACITY);

  mgr->next_tid = 0;
}

struct kTaskDsp *k_tmgr_get_free_task(struct kTaskDspMgr *mgr, uint8_t sz)
{
  void *stack_addr = pg_alloc_page(SYSADDR.pgmgr, sz, 0); // 16MB = 2^4 MB = 2^24 Bytes
  if (stack_addr == NULL)
  {
    DEBUG_PRINT("No Free Space for the stack!\r\n");
    return NULL;
  }

  struct kTaskDsp *free_task = mgr->free_list_head;
  if (free_task == NULL)
  {
    DEBUG_PRINT("No Free TaskDsp Left!\r\n");
    return NULL;
  }
  LL_DELETE2(mgr->free_list_head, free_task, next_free);

  free_task->stack_addr = (uintptr_t)stack_addr;
  free_task->stack_sz = 1 << sz;
  free_task->tid = mgr->next_tid;
  mgr->next_tid += 1;

  // Insert into hashtable tid->td
  HT_INSERT_KV(&mgr->map_tid_to_td, free_task->tid, free_task);

  return free_task;
}

void k_tmgr_destroy_task(struct kTaskDspMgr *mgr, struct kTaskDsp *td)
{
  // Remove from hash table tid->td
  HT_DEL_BY_KEY(&mgr->map_tid_to_td, &td->tid);

  pg_free_page(SYSADDR.pgmgr, (void *)td->stack_addr);

  // change any flags for td
  LL_PREPEND2(mgr->free_list_head, td, next_free);
  td->tid = K_TID_INVALID;
}

void k_td_init_user_task_default_wrapper(void (*user_func_addr)())
{
  DEBUG_PRINT("User Task Wrapper Started\r\n");
  user_func_addr();
  ke_exit();
}

void k_td_init_user_task(struct kTaskDsp *td, struct kTaskDsp *parent_td, uint64_t priority, void (*user_func_addr)())
{
  td->syscall_retval = (uint64_t)user_func_addr; // This gets extracted into x0, which will be treated as argument as well.
  td->pc = (uint64_t)k_td_init_user_task_default_wrapper;
  td->esr_el1 = 1; // as long as it is not 0
  td->pstate = 0;
  td->priority_key.priority = priority;

  td->sp = td->stack_addr + (uintptr_t)td->stack_sz;

  // simulate the reg_store when the user traps into the kernel
  struct kRegStore *user_reg_store = (struct kRegStore *)(td->sp - sizeof(struct kRegStore));
  util_memset(user_reg_store, 0x5A, sizeof(struct kRegStore)); // DEBUG
  td->sp = (uint64_t)user_reg_store;

  td->parent_tid = parent_td->tid;
  // when we create a task, is it always in the ready state?
  td->state = READY;

  k_mbox_init(&td->mailbox);
}

int k_td_get_syscall_no(struct kTaskDsp *kd)
{
  if (kd->esr_el1 == 0)
    return -1;
  return kd->esr_el1 & ((1 << 25) - 1);
}

void k_td_print(struct kTaskDsp *kc)
{
  char msg1[] = "Print kTaskDsp\r\n";
  uart_puts(0, 0, msg1, sizeof(msg1) - 1);
  uart_getc(0, 0);
  uart_puts(0, 0, print_reg(kc->pc), 18);
  uart_puts(0, 0, "\r\n", 2);
  uart_puts(0, 0, print_reg(kc->sp), 18);
  uart_puts(0, 0, "\r\n", 2);
}
