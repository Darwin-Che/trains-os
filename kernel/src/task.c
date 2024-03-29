#include "task.h"
#include "lib/include/rpi.h"
#include "util.h"
#include "lib/include/util.h"
#include "global_state.h"
#include "debug_print.h"
#include "lib/include/uapi.h"
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

// void k_td_init_user_task_default_wrapper(void (*user_func_addr)())
// {
//   DEBUG_PRINT("User Task Wrapper Started\r\n");
//   user_func_addr();
//   ke_exit();
// }

void k_td_init_user_task(struct kTaskDsp *td, struct kTaskDsp *parent_td, uint64_t priority,
                         void (*user_func_addr)(), const char *args, size_t args_len)
{
  td->pc = (uint64_t)user_func_addr;
  td->pstate = 0;
  td->priority_key.priority = priority;
  td->parent_tid = parent_td->tid;

  // when we create a task, is it always in the ready state?
  td->state = READY;

  // simulate the reg_store when the user had an interrupt (saves x0 in syscall_retval, x1-x31 on stack)
  td->esr_el1 = 0; // interrupt indicator
  td->sp = td->stack_addr + (uintptr_t)td->stack_sz;

  // copy the data on stack before the registers
  if (args_len != 0)
  {
    uint64_t alloc_len = (args_len & ~0xF) + ((args_len & 0xF) ? 0x10 : 0x0); // stack pointer must align at 16 bytes for aarch64
    td->sp -= alloc_len;
    util_memcpy((void *)td->sp, args, args_len);
  }

  // simulate the reg_store at the top of the stack
  struct kRegStore *user_reg_store = (struct kRegStore *)(td->sp - sizeof(struct kRegStore));
  util_memset(user_reg_store, 0x5A, sizeof(struct kRegStore)); // DEBUG

  // pass the data
  td->syscall_retval = td->sp; // This gets extracted into x0, which will be treated as argument 1 to user_func_addr.
  user_reg_store->x01 = args_len;
  user_reg_store->x30 = (uint64_t)ke_exit; // Always ends the process with ke_exit

  td->sp = (uint64_t)user_reg_store;

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
  printf("Print kTaskDsp\r\npc=%x\r\nsp=%s\r\n", kc->pc, kc->sp);
}
