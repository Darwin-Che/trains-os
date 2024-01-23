#include "bootstrap.h"
#include "sys_val.h"
#include "pgmgr_init.h"
#include "pgmgr.h"
#include "lib/include/slab.h"

extern void create_page_tables();
extern void set_page_tables();

extern char LINKER_CODE_START[];
extern char LINKER_CODE_END[];
extern char KERNEL_STACK_START[];
extern char KERNEL_STACK_END[];

static const uintptr_t FREE_START = 0x40000000;
static const uintptr_t FREE_END = 0x7E000000;

static struct PgInitSeg segs[] = {
    {
        .start_addr = (uintptr_t)0,
        .end_addr = (uintptr_t)LINKER_CODE_START,
        .flag = PGINIT_SEG_DSP_READ_ONLY,
    },
    {
        .start_addr = (uintptr_t)FREE_START,
        .end_addr = (uintptr_t)FREE_END,
        .flag = 0,
    },
};

void *k_bootstrap_pgmgr()
{
  /* Set up MMU */
  create_page_tables();
  set_page_tables();

  /* Initialize Page Manager */

  struct PgInitSegs segx = {
      .n = 2,
      .segs = segs,
  };

  struct PgMgr *pgmgr = pg_init(&segx);
  SYSADDR.pgmgr = pgmgr;

  return KERNEL_STACK_END;
  // return pg_alloc_page(pgmgr, KERNEL_STACK_SFT, 0);
}

void k_bootstrap_slab()
{
  // ALLOC = sizeof(struct SlabMgr); ~= 4096 Bytes
  SYSADDR.slabmgr = pg_alloc_page(SYSADDR.pgmgr, 12, 0);
  SYSADDR.slabmgr->slab_alloc_n = 0;
}