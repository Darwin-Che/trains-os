#include "pgmgr.h"
#include "pgmgr_init.h"
#include <string.h>
#include <stdio.h>
#include "lib/include/util.h"
#include "lib/include/printf.h"

static inline void memswap(void *const a, void *const b, const size_t n)
{
  unsigned char *p;
  unsigned char *q;
  unsigned char *const sentry = (unsigned char *)a + n;

  for (p = a, q = b; p < sentry; ++p, ++q)
  {
    const unsigned char t = *p;
    *p = *q;
    *q = t;
  }
}

static inline uintptr_t msbmask_ptr_t(uintptr_t x)
{
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  x |= (x >> 32);
  return (x & ~(x >> 1));
}

static uint32_t pg_pagecnt_maxlvl(uintptr_t num_pages);
static uint32_t pg_addr_maxlvl(uintptr_t addr);

/******************************************* pg_init_trimx *******************************************/
/*
Bubble sort?
[a,b,d,e] add c
->
[a,b,d,c]
[a,b,c,d]
*/
static void pg_init_trimx_init_empty(struct PgInitSegs *trimx);
static void pg_init_trimx_nonempty_cnt(struct PgInitSegs *trimx);
static bool pg_init_trimx_maybe_add(struct PgInitSegs *trimx, struct PgInitSeg *cand);
static uint32_t pg_init_trimx(struct PgInitSegs *segx, struct PgInitSegs *trimx)
{
  struct PgInitSeg cand;

  pg_init_trimx_init_empty(trimx);

  for (uint32_t i = 0; i < segx->n; i += 1)
  {
    struct PgInitSeg *seg = &segx->segs[i];

    if (seg->start_addr >= seg->end_addr)
      continue;

    uintptr_t start_addr_trim = ALIGN_UP(seg->start_addr, PG_SFT);
    uintptr_t end_addr_trim = ALIGN_DOWN(seg->end_addr, PG_SFT);

    if (end_addr_trim > start_addr_trim)
    {
      if (seg->start_addr != start_addr_trim)
      {
        pg_init_seg_ctor(&cand, seg->start_addr, start_addr_trim, seg->flag);
        pg_init_trimx_maybe_add(trimx, &cand);
      }

      if (seg->end_addr != end_addr_trim)
      {
        pg_init_seg_ctor(&cand, end_addr_trim, seg->end_addr, seg->flag);
        pg_init_trimx_maybe_add(trimx, &cand);
      }
    }
    else
    {
      pg_init_seg_ctor(&cand, seg->start_addr, seg->end_addr, seg->flag);
      pg_init_trimx_maybe_add(trimx, &cand);
    }

    seg->start_addr = start_addr_trim;
    seg->end_addr = end_addr_trim;
  }

  pg_init_trimx_nonempty_cnt(trimx);
  return trimx->n;
}

static void pg_init_trimx_init_empty(struct PgInitSegs *trimx)
{
  /*
    I want to achieve this

    uint32_t trims_n = trimx->n;
    struct PgInitSeg *trims = trimx->segs;
    for (int i = 0; i < trims_n; i += 1)
    {
      trims[i].start_addr = 0x0;
      trims[i].end_addr = 0x0;
      trims[i].flag = 0;
    }
  */
  memset(trimx->segs, 0x0, trimx->n * sizeof(struct PgInitSeg));
}

static void pg_init_trimx_nonempty_cnt(struct PgInitSegs *trimx)
{
  uint32_t trims_n = trimx->n;
  struct PgInitSeg *trims = trimx->segs;
  for (uint32_t i = 0; i < trims_n; i += 1)
  {
    if (trims[i].start_addr != trims[i].end_addr)
    {
      trimx->n = i;
      return;
    }
  }
}

static bool pg_init_trimx_maybe_add(struct PgInitSegs *trimx, struct PgInitSeg *cand)
{
  uint32_t trims_n = trimx->n;
  struct PgInitSeg *trims = trimx->segs;

  if (cand->end_addr - cand->start_addr <= trims[trims_n - 1].end_addr - trims[trims_n - 1].start_addr)
    return false;

  memcpy(&trims[trims_n - 1], cand, sizeof(struct PgInitSeg));

  for (int i = trimx->n - 2; i >= 0; i -= 1)
  {
    if (trims[i].end_addr - trims[i].start_addr >= trims[i + 1].end_addr - trims[i + 1].start_addr)
      break;
    memswap(&trims[i], &trims[i + 1], sizeof(struct PgInitSeg));
  }

  return true;
}

/******************************************* pg_init_rootx *******************************************/

static uint32_t pg_init_rootx(struct PgInitSegs *segx, struct PgRoot *roots)
{
  uint32_t roots_n = 0;

  for (uint32_t i = 0; i < segx->n; i += 1)
  {
    struct PgInitSeg *seg = &segx->segs[i];

    if (seg->start_addr >= seg->end_addr)
      continue;

    if (roots)
    {
      roots[roots_n].start_addr = seg->start_addr;
      roots[roots_n].end_addr = seg->end_addr;
      roots[roots_n].root = NULL;
    }

    roots_n += 1;
  }

  return roots_n;
}

static uint32_t pg_addr_maxlvl(uintptr_t addr)
{
  uint32_t lvl = PGMGR_MAXLVL;
  for (; lvl > 0; lvl -= 1)
  {
    if (0 == (addr & ((1 << (lvl + PG_SFT)) - 1)))
      return lvl;
  }
  return lvl;
}

/******************************************* pg_init_pgmgr_addr *******************************************/

static uintptr_t pg_init_pgmgr_addr(const struct PgInitSegs *segx, uint32_t pgmgr_sz)
{
  uintptr_t pgmgr_addr = UINTPTR_MAX;
  uintptr_t rating = UINTPTR_MAX;

  for (uint32_t i = 0; i < segx->n; i += 1)
  {
    const struct PgInitSeg *seg = &segx->segs[i];

    if (seg->flag & PGINIT_SEG_DSP_READ_ONLY)
      continue;

    uintptr_t aligned_start = ALIGN_UP(seg->start_addr, CACHE_LINE_SFT);

    if (aligned_start + pgmgr_sz > seg->end_addr)
      continue;

    uintptr_t new_rating = seg->end_addr - aligned_start;

    if (pgmgr_addr == 0 || new_rating < rating)
    {
      rating = new_rating;
      pgmgr_addr = aligned_start;
    }
  }

  return pgmgr_addr;
}

/******************************************* pg_init_pgdsps *******************************************/

static int32_t pg_init_pgdsps_next_root(struct PgMgr *pgmgr, struct PgInitSegs *segx);
static bool pg_init_pgdsps_alloc_root(struct PgMgr *pgmgr, struct PgInitSegs *segx, uint32_t idx);
static uintptr_t pg_init_pgdsps_calc_space_for_root(const struct PgRoot *root);
static void pg_init_pgdsps_write_root(struct PgMgr *pgmgr, uint32_t idx);

/*
Heuristic:
  Allocate the PgDsps for the roots from smallest root to the largest root, (of course the unavailable root comes last)
  Also put the PgDsps into the smallest blank space first.
*/
static bool pg_init_pgdsps(struct PgMgr *pgmgr, struct PgInitSegs *segx)
{
  while (1)
  {
    int32_t idx = pg_init_pgdsps_next_root(pgmgr, segx);
    if (idx < 0)
      break;

    if (!pg_init_pgdsps_alloc_root(pgmgr, segx, (uint32_t)idx))
      return false;

    pg_init_pgdsps_write_root(pgmgr, (uint32_t)idx);
  }

  return true;
}

/*
Heuristic:
  choose the smallest root
*/
static int32_t pg_init_pgdsps_next_root(struct PgMgr *pgmgr, struct PgInitSegs *segx)
{
  int32_t idx = -1;
  uintptr_t smallest_score = UINTPTR_MAX;

  for (uint32_t i = 0; i < pgmgr->roots_n; i += 1)
  {
    // Skip those that are already allocated
    if (pgmgr->roots[i].root != NULL)
      continue;

    if (segx->segs[i].flag & PGINIT_SEG_DSP_READ_ONLY)
    {
      // those that are read-only will be alloced at last
      if (idx == -1)
        idx = i;
      continue;
    }

    uintptr_t this_score = (pgmgr->roots[i].end_addr - pgmgr->roots[i].start_addr) >> PG_SFT;
    if (this_score < smallest_score)
    {
      smallest_score = this_score;
      idx = i;
    }
  }

  return idx;
}

/*
Heuristic:
  Among
  1) Already allocated segments
  2) segment at idx
  Choose the smallest section that can accomodate the size needed for idx
*/
static bool pg_init_pgdsps_alloc_root(struct PgMgr *pgmgr, struct PgInitSegs *segx, uint32_t pgidx)
{
  int32_t best_idx = -1;
  uintptr_t best_space = UINTPTR_MAX;
  uintptr_t require_space = pg_init_pgdsps_calc_space_for_root(&pgmgr->roots[pgidx]);

  uintptr_t blank_space;

  for (uint32_t i = 0; i < pgmgr->roots_n; i += 1)
  {
    // Skip if the segment is read-only
    if (segx->segs[i].flag & PGINIT_SEG_DSP_READ_ONLY)
      continue;

    // Skip if this segment is not allocated yet and i != pgidx
    if (i != pgidx && pgmgr->roots[i].root == NULL)
      continue;

    blank_space = (segx->segs[i].end_addr - segx->segs[i].start_addr);

    // Skip if this segment is not big enough for the root
    if (blank_space < require_space)
      continue;

    if (blank_space < best_space)
    {
      best_space = blank_space;
      best_idx = i;
    }
  }

  if (best_idx >= 0)
  {
    pgmgr->roots[pgidx].root = (struct PgDsp *)segx->segs[best_idx].start_addr;
    segx->segs[best_idx].start_addr += require_space;
    return true;
  }
  return false;
}

static void pg_init_pgdsps_write_root(struct PgMgr *pgmgr, uint32_t idx)
{
  uintptr_t num_pages = (pgmgr->roots[idx].end_addr - pgmgr->roots[idx].start_addr) >> PG_SFT;
  memset(pgmgr->roots[idx].root, 0x0, num_pages * sizeof(struct PgDsp));
  for (uintptr_t i = 0; i < num_pages; i += 1)
  {
    pgmgr->roots[idx].root[i].addr = pgmgr->roots[idx].start_addr + (i << PG_SFT);
  }
}

static uintptr_t pg_init_pgdsps_calc_space_for_root(const struct PgRoot *root)
{
  return ((root->end_addr - root->start_addr) >> PG_SFT) * sizeof(struct PgDsp);
}

/******************************************* pg_init_blankspace *******************************************/

/*
Rearrange & cut segx so that pgmgr->roots[i] is the same seg as segx->segs[i].
Also segx->segx[i].flag is preserved.
*/
static bool pg_init_blankspace(struct PgMgr *pgmgr, struct PgInitSegs *segx, uintptr_t pgmgr_sz)
{
  // By construction, segx always contains more elements thatn pgmgr->roots
  // (In function `pg_init_rootx`)
  for (uint32_t i = 0; i < pgmgr->roots_n; i += 1)
  {
    struct PgRoot *root = &pgmgr->roots[i];

    uint32_t j = i;
    for (; j < segx->n; j += 1)
    {
      // find the seg corresponding to this root
      struct PgInitSeg *seg = &segx->segs[j];
      if (seg->start_addr <= root->start_addr && seg->end_addr >= root->end_addr)
        break;
    }

    if (j == segx->n)
      return false;

    if (j != i)
    {
      memswap(&segx->segs[i], &segx->segs[j], sizeof(struct PgInitSeg));
    }

    segx->segs[i].start_addr = root->start_addr;
    segx->segs[i].end_addr = root->end_addr;

    // if this segment contains PgMgr, need to mark those space being used
    if ((uintptr_t)pgmgr >= segx->segs[i].start_addr && (uintptr_t)pgmgr < segx->segs[i].end_addr)
    {
      segx->segs[i].start_addr = (uintptr_t)pgmgr + pgmgr_sz;
    }
  }

  segx->n = pgmgr->roots_n;
  return true;
}

/******************************************* pg_init_free_lists *******************************************/

static void pg_init_free_lists_per_root(struct PgMgr *pgmgr, struct PgRoot *root, struct PgInitSeg *seg);
static void pg_init_free_lists_add_range(struct PgMgr *pgmgr, struct PgRoot *root, uintptr_t start_idx, uintptr_t end_idx);

static void pg_init_free_lists(struct PgMgr *pgmgr, struct PgInitSegs *segx)
{
  for (uint32_t i = 0; i < pgmgr->roots_n; i += 1)
  {
    pg_init_free_lists_per_root(pgmgr, &pgmgr->roots[i], &segx->segs[i]);
  }
}

static void pg_init_free_lists_per_root(struct PgMgr *pgmgr, struct PgRoot *root, struct PgInitSeg *seg)
{
  // Mark the region outside of [seg->start_addr, seg->end_addr] to be Dsp Used
  // Otherwise, mark the region as free and add to free_list as needed

  seg->start_addr = ALIGN_UP(seg->start_addr, PG_SFT);
  seg->end_addr = ALIGN_DOWN(seg->end_addr, PG_SFT);

  uintptr_t num_pages = (root->end_addr - root->start_addr) >> PG_SFT;

  uintptr_t start_idx = 0, end_idx = num_pages;

  for (uintptr_t i = 0; i < num_pages; i += 1)
  {
    struct PgDsp *dsp = &root->root[i];

    if (dsp->addr == seg->start_addr)
      start_idx = i;
    if (dsp->addr == seg->end_addr)
      end_idx = i;

    if (dsp->addr < seg->start_addr || dsp->addr >= seg->end_addr)
      dsp->flags |= PGDSP_USED;
  }

  // Add to free_lists
  pg_init_free_lists_add_range(pgmgr, root, start_idx, end_idx);
}

/*
  Seperate [start_addr, end_addr] in to roots that are aligned with power of 2
  Example
  [ 0x0101, 0x1001 ]
  ->
  [ 0x0101, 0x0110 ]
  [ 0x0110, 0x1000 ]
  [ 0x1000, 0x1001 ]
  Example
  [ 0x0100, 0x0111 ]
  ->
  [ 0x0100, 0x0110 ]
  [ 0x0110, 0x0111 ]

  It is clear what algorithm we should take here
  1. Take the largest level for start_addr
  2. Take a root if that largest level is fully contained in end_addr, and repeat
  3. Take end_addr - start_addr, in 2's power is the leftover roots, e.g. 0x101 means 0x100 and 0x1
*/
static void pg_init_free_lists_add_range(struct PgMgr *pgmgr, struct PgRoot *root, uintptr_t start_idx, uintptr_t end_idx)
{
  while (1)
  {
    uintptr_t start_addr = root->root[start_idx].addr;
    uint32_t lvl = pg_addr_maxlvl(start_addr);

    if (start_idx + (1 << lvl) > end_idx)
      break;

    DL_APPEND2(pgmgr->free_lists[lvl].head, &root->root[start_idx], free_list_prev, free_list_next);

    start_idx += (1 << lvl);
  }

  while (start_idx != end_idx)
  {
    uintptr_t start_addr = root->root[start_idx].addr;
    uint32_t lvl_by_addr = pg_addr_maxlvl(start_addr);
    uint32_t lvl_by_pagecnt = pg_pagecnt_maxlvl(end_idx - start_idx);
    uint32_t lvl = MIN(lvl_by_addr, lvl_by_pagecnt);

    DL_APPEND2(pgmgr->free_lists[lvl].head, &root->root[start_idx], free_list_prev, free_list_next);

    start_idx += (1 << lvl);
  }
}

static uint32_t pg_pagecnt_maxlvl(uintptr_t num_pages)
{
  uint32_t lvl = PGMGR_MAXLVL;
  for (; lvl > 0; lvl -= 1)
  {
    if (num_pages & (1 << lvl))
      return lvl;
  }
  return lvl;
}

/******************************************* pg_init *******************************************/

/*
Takes [s1, e1], [s2, e2], [s3, e3], ...
All segs are non-overlapping, and sorted ascendingly.
Specify which segment are available for use by the PgDsp.

Trim those that are not aligned at a page
e.g. 0x1100
Those areas that are not a full page are a potential place for PgDsp.

After trim, iterate through the segs, and obtain roots.
Knowing how many roots are there, we want to select the region of PgDsp and PgMgr.
With the following preference (after allocation of PgDsp):
  1. The largest block to be as large as possible
  2. The number of largest block to be as many as possible

Formal abstraction of this problem:
Input: Array of roots with [start, end]; a number of allocations
Output: Address for those allocations
Achieving:
  1. Among all allocations, the leftover of the roots have largest block (need to align with power of 2)
  2. Among all allocations, the number of largest leftover block is biggest

Conclusion:
  Not quite solvable.

Hueristic:
  Allocate the PgDsps for the roots from smallest root to the largest root, (of course the unavailable root comes last)
  Also put the PgDsps into the smallest blank space first.

Things to consider,
1. Suppose we have only a few pages, We don't want to waste a whole page
just to allocate a few PgDsps. Is there a way to let the rest of the spaces be used?
--> Let the user pick page size, to reduce waste.
2. Since at this time, no dynamic memory usage is easy/feasible.
This init function should have an upperbound on the stack size.
A reasonable upperbound is counted in the multiple of pages.
The first attempt is to restrict the stack to one page.
*/

// #define PGINIT_TRIMS_ENABLED
#define PGINIT_TRIMS_N 8

struct PgMgr *pg_init(struct PgInitSegs *segx)
{
  // an array trim-offs after trim
  struct PgInitSeg trims[PGINIT_TRIMS_N];
  struct PgInitSegs trimx = {.n = PGINIT_TRIMS_N, .segs = trims};
  pg_init_trimx(segx, &trimx);

  // estimate how large is PgMgr by counting the number of needed roots
  // Maintain the logic of roots computation at one place, easier to maintain and obtain same results.
  uint32_t roots_n = pg_init_rootx(segx, NULL);
  uint32_t pgmgr_sz = sizeof(struct PgMgr) + sizeof(struct PgRoot) * roots_n;

  // Allocate and write the section for PgMgr
  uintptr_t pgmgr_addr = pg_init_pgmgr_addr(segx, pgmgr_sz);
  if (pgmgr_addr == UINTPTR_MAX)
    return NULL;
  // Init the PgMgr itself
  struct PgMgr *pgmgr = (struct PgMgr *)pgmgr_addr;
  memset(pgmgr, 0x0, sizeof(struct PgMgr));
  // Init the following roots list
  pgmgr->roots_n = roots_n;
  if (pg_init_rootx(segx, pgmgr->roots) != roots_n)
    return NULL;

  // Init the segx to be an array of blank space
  if (!pg_init_blankspace(pgmgr, segx, pgmgr_sz))
    return NULL;

  // Allocate and write the section for the PgDsp array for each root
  if (!pg_init_pgdsps(pgmgr, segx))
    return NULL;

  // Setup the free_lists, also consider the spaces used by the PgDsp themselves
  pg_init_free_lists(pgmgr, segx);

  return pgmgr;
}

/*
Want to allocate (1 << p) bytes
*/
void *pg_alloc_page(struct PgMgr *mgr, uint8_t p, uint16_t flags)
{
  (void)flags;
  // Interpret p as level of pages
  // p = 0 -> p = 0
  // p = 6 -> p = 0
  // p = 12 -> p = 0
  // p = 13 -> p = 1
  // p = 16 -> p = 4
  // starting from level p (increment), find the first level with non-empty free_list
  p = MAX(0, p - PG_SFT);
  uint8_t q = p;
  for (; q <= PGMGR_MAXLVL; q++)
  {
    if (mgr->free_lists[q].head != NULL)
      break;
  }

  if (q > PGMGR_MAXLVL)
    return NULL;

  // break up the free_list until level p
  struct PgDsp *dsp = mgr->free_lists[q].head;
  DL_DELETE2(mgr->free_lists[q].head, dsp, free_list_prev, free_list_next);
  for (; q > p; q--)
  {
    struct PgDsp *unused_half = dsp + (1 << (q - 1));
    unused_half->flags &= ~PGDSP_USED; // actually not needed
    DL_APPEND2(mgr->free_lists[q - 1].head, unused_half, free_list_prev, free_list_next);
  }

  // set flag and lvl of this dsp
  dsp->flags |= PGDSP_USED;
  dsp->lvl = p;
  return (void *)dsp->addr;
}

static inline bool addr_in_segment(struct PgRoot *seg, uintptr_t addr)
{
  return addr >= seg->start_addr && addr < seg->end_addr;
}

void pg_free_page(struct PgMgr *mgr, void *ptr)
{
  // identify the segment of addr
  unsigned l = 0, r = mgr->roots_n, m;
  // binary search with [l,r] being the range
  while (l < r)
  {
    m = (l + r) / 2; // no chances of overflow
    if ((uintptr_t)ptr < mgr->roots[m].start_addr)
      r = m - 1;
    else if ((uintptr_t)ptr >= mgr->roots[m].end_addr)
      l = m + 1;
    else
      break;
  }

  struct PgRoot *seg = &mgr->roots[l];

  // addr is not a valid addr, this is an error
  if (!addr_in_segment(seg, (uintptr_t)ptr))
    return;

  // index the PgDsp of addr
  struct PgDsp *dsp = seg->root;
  dsp += (((uintptr_t)ptr - seg->start_addr) >> PG_SFT);

  // Look at the lvl of this PgDsp
  uint8_t lvl = dsp->lvl;

  // From that lvl up, look at corresponding buddy (if exists in the segment)
  // If that buddy is free, remove buddy from free_list, combine and lvl++ to check more
  for (; lvl < PGMGR_MAXLVL; lvl++)
  {
    if (dsp->addr & (1 << (PG_SFT + lvl - 1)))
      break;

    uintptr_t buddy_addr = dsp->addr ^ (1 << (PG_SFT + lvl));
    if (!addr_in_segment(seg, buddy_addr))
      break;

    struct PgDsp *buddy;
    if (dsp->addr & (1 << (PG_SFT + lvl)))
      buddy = dsp - (1 << lvl);
    else
      buddy = dsp + (1 << lvl);

    if (buddy->flags & PGDSP_USED)
      break;

    DL_DELETE2(mgr->free_lists[lvl].head, buddy, free_list_prev, free_list_next);

    if (buddy < dsp)
      dsp = buddy;
  }

  // insert this current PgDsp into the lvl free_list
  DL_APPEND2(mgr->free_lists[lvl].head, dsp, free_list_prev, free_list_next);
}

/******************************************* pgmgr_debug_* *******************************************/

void pgmgr_debug_print(const struct PgMgr *mgr, bool verbose)
{
  printf("  | ========== pgmgr_debug_print ==========\r\n");

  printf("  | pgmgr = %p\r\n", mgr);
  if (mgr == NULL)
    return;

  printf("  | roots_n = %d\r\n", mgr->roots_n);

  for (uint32_t i = 0; i < mgr->roots_n; i += 1)
  {
    printf("  |   roots[%d] = %p\r\n", i, &mgr->roots[i]);
    printf("  |   roots[%d] = [%lx, %lx] -> %p\r\n", i, mgr->roots[i].start_addr, mgr->roots[i].end_addr, mgr->roots[i].root);

    if (verbose)
    {
      uintptr_t num_pages = (mgr->roots[i].end_addr - mgr->roots[i].start_addr) >> PG_SFT;
      for (uint32_t j = 0; j < num_pages; j += 1)
      {
        printf("  |     pgmgr.roots[%3d].dsp[%3d] @ %p = %lx", i, j, &mgr->roots[i].root[j], mgr->roots[i].root[j].addr);
        if (mgr->roots[i].root[j].flags & PGDSP_USED)
          printf(" , USED");
        printf("\r\n");
      }
    }
  }

  printf("  | ---------- free_list ----------\r\n");
  for (int32_t i = PGMGR_MAXLVL; i >= 0; i -= 1)
  {
    printf("  | LVL = %2d, SZ = %10x\r\n", i, (1 << (i + PG_SFT)));
    struct PgDsp *elem;
    uintptr_t cnt = 0;
    DL_FOREACH2(mgr->free_lists[i].head, elem, free_list_next)
    {
      printf("  |   %2lu : [%lx, %lx] FLAG=%x\r\n", cnt, elem->addr, elem->addr + (1 << (i + PG_SFT)), elem->flags);
      cnt += 1;
    }
  }
}

void pgmgr_debug(const struct PgMgr *mgr, struct PgMgrDebug *debug)
{
  debug->mgr = (struct PgMgr *)mgr;
  if (mgr == NULL)
    return;

  debug->roots_n = MIN(debug->roots_n, mgr->roots_n);

  for (uint32_t i = 0; i < debug->roots_n; i += 1)
  {
    uintptr_t num_pages = (mgr->roots[i].end_addr - mgr->roots[i].start_addr) >> PG_SFT;
    debug->roots_num_pages[i] = num_pages;

    uintptr_t used_pages = 0;
    for (uint32_t j = 0; j < num_pages; j += 1)
    {
      if (mgr->roots[i].root[j].flags & PGDSP_USED)
        used_pages += 1;
    }
    debug->roots_num_pages_used[i] = used_pages;
  }

  for (int32_t i = PGMGR_MAXLVL; i >= 0; i -= 1)
  {
    struct PgDsp *elem;
    uintptr_t cnt = 0;
    DL_FOREACH2(mgr->free_lists[i].head, elem, free_list_next)
    {
      cnt += 1;
    }
    debug->free_lists_cnt[i] = cnt;
  }

  debug->roots_n = mgr->roots_n;
}
