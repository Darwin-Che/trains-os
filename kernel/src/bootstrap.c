#include "bootstrap.h"
#include "sys_val.h"
#include "pgmgr_init.h"

extern void create_page_tables();
extern void set_page_tables();

extern char LINKER_CODE_START[];
extern char LINKER_CODE_END[];

static const uintptr_t FREE_START = 0x40000000;
static const uintptr_t FREE_END = 0x80000000;

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

void k_bootstrap()
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
}