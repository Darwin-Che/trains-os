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
#include "slab_kernel.h"
#include "lib/include/slab.h"
#include "lib/include/rbtree.h"

void k_tmgr_init(struct kTaskDspMgr *mgr)
{
  mgr->task_alloc = slab_create(SYSADDR.slabmgr, sizeof(struct kTaskDsp), _Alignof(struct kTaskDsp), slab_kernel_func_alloc_page);

  ASSERT_MSG(mgr->task_alloc != NULL, "slab_create gives NULL\n");

#ifdef DEBUG
  printf("mgr->task_alloc = %p\n", mgr->task_alloc);
#endif

  mgr->map_tid = RB_ROOT;

  mgr->next_tid = 0;
}

struct kTaskDsp *k_tmgr_get_free_task(struct kTaskDspMgr *mgr, uint8_t sz)
{
  struct kTaskDsp *free_task = slab_alloc(mgr->task_alloc, 0);
  if (free_task == NULL)
  {
    DEBUG_PRINT("No Free TaskDsp Left!\r\n");
    return NULL;
  }

  void *stack_addr = pg_alloc_page(SYSADDR.pgmgr, sz, 0); // 16MB = 2^4 MB = 2^24 Bytes
  if (stack_addr == NULL)
  {
    DEBUG_PRINT("No Free Space for the stack!\r\n");
    slab_free(mgr->task_alloc, free_task);
    return NULL;
  }

  free_task->stack_addr = (uintptr_t)stack_addr;
  free_task->stack_sz = 1 << sz;
  free_task->tid = mgr->next_tid;
  mgr->next_tid += 1;

  // Insert into rbtree tid->td
  struct rb_node *node = rb_find_add(&free_task->rb_link_tid, &mgr->map_tid, k_td_rb_cmp_tid);
  if (node != NULL)
  {
    DEBUG_PRINT("Duplicate task id %ld!\r\n", free_task->tid);
    slab_free(mgr->task_alloc, free_task);
    return NULL;
  }

  DEBUG_PRINT("k_tmgr_get_free_task succeeds!\r\n");
  return free_task;
}

void k_tmgr_destroy_task(struct kTaskDspMgr *mgr, struct kTaskDsp *td)
{
  // Remove from rbtree tid->td
  rb_erase(&td->rb_link_tid, &mgr->map_tid);

  pg_free_page(SYSADDR.pgmgr, (void *)td->stack_addr);

  // change any flags for td
  slab_free(mgr->task_alloc, td);
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
