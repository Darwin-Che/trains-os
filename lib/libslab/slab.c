#include "slab.h"
#include "math.h"
#include "printf.h"
#include "utlist.h"
#include "../../kernel/include/pgmgr.h"

static void slab_slab_set_magic(struct Slab *slab)
{
  slab->magic[0] = 's';
  slab->magic[1] = 'l';
  slab->magic[2] = 'a';
  slab->magic[3] = 'b';
}

static bool slab_slab_check_magic(struct Slab *slab)
{
  return (
      slab->magic[0] == (uint8_t)'s' &&
      slab->magic[1] == (uint8_t)'l' &&
      slab->magic[2] == (uint8_t)'a' &&
      slab->magic[3] == (uint8_t)'b');
}

static void slab_slab_init(struct Slab *slab, struct SlabAlloc *alloc)
{
  slab_slab_set_magic(slab);

  slab->alloc = alloc;

  uint16_t obj_n = slab->alloc->obj_n;

  for (uint16_t i = 0; i < obj_n - 1; i += 1)
  {
    slab->free_list[i] = i + 1;
  }
  slab->free_list[obj_n - 1] = obj_n - 1;
  slab->first_free = 0;

  slab->used_cnt = 0;
  slab->prev_slab = NULL;
  slab->next_slab = NULL;

  uint32_t padding = slab->alloc->padding_base;
  uint32_t rand = math_rand_u32();
  rand = rand % (SLAB_END_PADDING_MIN / slab->alloc->padding_align);
  padding += rand * slab->alloc->padding_align;

  slab->data = (void *)(((uintptr_t)slab) + slab->alloc->obj_n + padding);
}

static uint32_t slab_alloc_calc_page_sft(uint32_t obj_sz)
{
  (void)obj_sz;
  return PG_SFT;
}

static bool slab_create_allow(uint32_t obj_sz, uint32_t obj_align)
{
  (void)obj_align;
  return obj_sz >= 16 && obj_sz < PG_SZ / 2;
}

static bool slab_alloc_init(struct SlabAlloc *alloc, uint32_t obj_sz, uint32_t obj_align)
{
  alloc->obj_sz = obj_sz;
  alloc->pg_sft = slab_alloc_calc_page_sft(obj_sz);
  alloc->empty_slabs = NULL;
  alloc->slabs = NULL;

  uint32_t obj_align_sft = MSB_POS(obj_align);

  /*
  | sizeof(Slab) | obj_n * 1 | padding | obj_n * obj_sz | end_padding |
  padding + end_padding = PG_SZ - (sizeof(Slab) + obj_n * (1 + obj_sz))
  sizeof(Slab) + obj_n * 1 + padding divides obj_align
  padding, end_padding >= 0
  UNKNOWN obj_n, padding, end_padding
  MAX obj_n
  MIN / MAX padding
  */

  uint64_t max_obj_n = (PG_SZ - sizeof(struct Slab)) / (1 + obj_sz);
  if (max_obj_n > SLAB_OBJ_N_MAX)
    max_obj_n = SLAB_OBJ_N_MAX;

  uint16_t obj_n = max_obj_n;
  for (; obj_n > 0; obj_n -= 1)
  {
    // TEST for each obj_n, if the padding is enough
    uint32_t padding = sizeof(struct Slab) + obj_n;
    padding = ALIGN_UP(padding, obj_align_sft) - padding;

    uint32_t total_padding = PG_SZ - sizeof(struct Slab) - (1 + obj_sz) * obj_n;
    if (total_padding >= padding + SLAB_END_PADDING_MIN) // total_padding - padding >= SLAB_END_PADDING_MIN
    {
      alloc->obj_n = obj_n;
      alloc->padding_align = obj_align;
      alloc->padding_base = padding;
      break;
    }
  }

  if (obj_n == 0)
    return false;

  return true;
}

struct SlabAlloc *slab_create(struct SlabMgr *mgr, uint32_t obj_sz, uint32_t obj_align, void *(*func_alloc_page)(uint8_t, uint16_t))
{
  if (mgr->slab_alloc_n >= SLABMGR_CAP)
  {
    printf("slab_create capacity reached %u\n", mgr->slab_alloc_n);
    return NULL;
  }

  if (!slab_create_allow(obj_sz, obj_align))
  {
    printf("slab_create_allow failed %u %u\n", obj_sz, obj_align);
    return NULL;
  }

  struct SlabAlloc *alloc = &mgr->slab_alloc[mgr->slab_alloc_n];
  mgr->slab_alloc_n += 1;

  if (!slab_alloc_init(alloc, obj_sz, obj_align))
  {
    printf("slab_alloc_init failed %p %u %u\n", alloc, obj_sz, obj_align);
    return NULL;
  }

  alloc->func_alloc_page = func_alloc_page;

  return alloc;
}

static void *slab_slab_alloc(struct Slab *slab, uint32_t flag)
{
  (void)flag;
  if (slab->used_cnt >= slab->alloc->obj_n)
    return NULL;

  if (slab->first_free == -1)
    return NULL;

  void *ptr = (void *)((uintptr_t)slab->data + slab->alloc->obj_sz * slab->first_free);

  uint8_t first_free = slab->first_free;

  if (slab->free_list[first_free] == first_free)
    slab->first_free = -1;
  else
    slab->first_free = slab->free_list[first_free];

  slab->used_cnt += 1;
  return ptr;
}

static void slab_slab_free(struct Slab *slab, void *ptr)
{
  uint8_t idx = ((uintptr_t)ptr - (uintptr_t)slab->data) / slab->alloc->obj_sz;

  if (slab->first_free == -1)
    slab->free_list[idx] = idx;
  else
    slab->free_list[idx] = slab->first_free;

  slab->first_free = idx;
  slab->used_cnt -= 1;
}

void *slab_alloc(struct SlabAlloc *alloc, uint32_t flag)
{
  struct Slab *slab = alloc->slabs;

  // Check if there is already some unfilled pages
  if (slab == NULL || slab->used_cnt == alloc->obj_n)
  {
    // Check if there are some empty slabs on the empty list
    if (alloc->empty_slabs != NULL)
    {
      slab = alloc->empty_slabs;
      DL_DELETE2(alloc->empty_slabs, slab, prev_slab, next_slab);
    }
    else
    {
      // Need to request a new page
      slab = alloc->func_alloc_page(alloc->pg_sft, 0);
      if (slab == NULL)
        return NULL;
      slab_slab_init(slab, alloc);
    }

    DL_PREPEND2(alloc->slabs, slab, prev_slab, next_slab);
  }

  void *addr = slab_slab_alloc(slab, flag);

  if (slab->used_cnt == alloc->obj_n)
  {
    DL_DELETE2(alloc->slabs, slab, prev_slab, next_slab);
    DL_APPEND2(alloc->slabs, slab, prev_slab, next_slab);
  }

  return addr;
}

void slab_free(struct SlabAlloc *alloc, void *ptr)
{
  uint32_t pg_sft = alloc->pg_sft;
  struct Slab *slab = (struct Slab *)ALIGN_DOWN((uintptr_t)ptr, pg_sft);

  if (!slab_slab_check_magic(slab))
  {
    // PANIC();
    return;
  }

  slab_slab_free(slab, ptr);

  if (slab->used_cnt == 0)
  {
    // move the slab to the empty list
    DL_DELETE2(alloc->slabs, slab, prev_slab, next_slab);
    DL_PREPEND2(alloc->empty_slabs, slab, prev_slab, next_slab);
  }
  else if (slab != alloc->slabs)
  {
    // move the slab to the hot front
    DL_DELETE2(alloc->slabs, slab, prev_slab, next_slab);
    DL_PREPEND2(alloc->slabs, slab, prev_slab, next_slab);
  }
}

void slab_debug_print(const struct SlabAlloc *alloc)
{
  printf("  | ========== slab_debug_print alloc=%p ==========\n", alloc);
  if (alloc == NULL)
    return;
  printf("  | pg_sft=%d\n", alloc->pg_sft);
  printf("  | obj_n=%d\n", alloc->obj_n);
  printf("  | obj_sz=%d\n", alloc->obj_sz);
  printf("  | padding_base=%d\n", alloc->padding_base);
  printf("  | padding_align=%d\n", alloc->padding_align);

  struct Slab *slab;

  printf("  | ---------- slabs ----------\n");
  DL_FOREACH2(alloc->slabs, slab, next_slab)
  {
    printf("    | used_cnt=%d first_free=%d slab=%p data=%p\n", slab->used_cnt, slab->first_free, slab, slab->data);
  }

  printf("  | ---------- empty_slabs ----------\n");
  DL_FOREACH2(alloc->empty_slabs, slab, next_slab)
  {
    printf("    | slab=%p\n", slab);
  }
}