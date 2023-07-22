#include <stddef.h>
#include <stdint.h>

#define PGINIT_SEG_DSP_READ_ONLY (1 << 0)

struct PgInitSeg
{
  uintptr_t start_addr;
  uintptr_t end_addr;
  uint32_t flag;
};

static inline void pg_init_seg_ctor(struct PgInitSeg *seg, uintptr_t start_addr,
                                    uintptr_t end_addr, uint32_t flag)
{
  seg->start_addr = start_addr;
  seg->end_addr = end_addr;
  seg->flag = flag;
}

struct PgInitSegs
{
  uint32_t n;
  struct PgInitSeg *segs;
};

struct PgMgr *pg_init(struct PgInitSegs *segx);