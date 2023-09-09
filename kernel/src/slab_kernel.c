#include "lib/include/slab.h"
#include "sys_val.h"
#include "pgmgr.h"

void *slab_kernel_func_alloc_page(uint8_t p, uint16_t flags)
{
  return pg_alloc_page(SYSADDR.pgmgr, p, flags);
}
