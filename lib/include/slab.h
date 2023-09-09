#ifndef SLAB_H
#define SLAB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MSB_POS(x)                                                             \
  {                                                                            \
    uint8_t pos = sizeof(x) * 8 - 1;                                           \
    for (; pos > 0; pos -= 1) {                                                \
      if (x & (1 << pos))                                                      \
        break;                                                                 \
    }                                                                          \
    pos;                                                                       \
  }

struct SlabMeta {
  // In order to prevent cache imbalance, try to make obj_size
  //  - Conform to obj_align
  //  - If possible, not a multiple of 32 Bytes (Minimum cache line size)
  uint32_t obj_sz;
  uint32_t obj_align;
  uint32_t pg_sft;
};

/*
  Represents one slab, corresponding to a page (different sizes might exist)
  Belongs to a SlabAlloc
*/
struct Slab {
  uint8_t magic[4];
  void *data;
  struct SlabAlloc *alloc;
  struct Slab *prev_slab;
  struct Slab *next_slab;
  uint16_t used_cnt;  // this is the number of used objects in this slab
  int16_t first_free; // The first free object index [-1, obj_n]
  // uint8_t ctr_bitmap[256 / 8]; // A bit map for whether the ctor is called
  uint8_t free_list[]; // A list by storing the next index, free_list[i] = i
                       // means this is the list end
};

/*
  Represents the slab allocator for a single type of object
*/
#define SLAB_OBJ_N_MAX 256
#define SLAB_END_PADDING_MIN 64
struct SlabAlloc {
  uint32_t obj_sz;      // The size of the object, if the
  uint32_t pg_sft;      // The page size of the slabs
  uint16_t obj_n;       // the number of objects in the slab, the MAX is 256
  uint8_t padding_base; // The base padding
  uint8_t
      padding_align; // The number of bytes the padding needs to be aligned to

  // A dlist of slabs, first half is with room, second half is full. front is
  // hot on free, end is hot on alloc
  struct Slab *slabs;
  // A dlist of empty slabs - for more buffer, front is more recently used
  struct Slab *empty_slabs;

  // int func_ctr(void *);     // The constructor function
  // The function to request a page
  void *(*func_alloc_page)(uint8_t, uint16_t);
};

/*
  Manages the slabs, mainly used for bookkeeping
*/
#define SLABMGR_CAP 64
struct SlabMgr {
  struct SlabAlloc slab_alloc[SLABMGR_CAP];
  uint32_t slab_alloc_n;
};

struct SlabAlloc *slab_create(struct SlabMgr *mgr, uint32_t obj_sz,
                              uint32_t obj_align);

void *slab_alloc(struct SlabAlloc *alloc, uint32_t flag);

void slab_free(struct SlabAlloc *alloc, void *ptr);

void slab_debug_print(const struct SlabAlloc *alloc);

#endif