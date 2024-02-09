#ifndef PGMGR_H
#define PGMGR_H

#include "lib/include/utlist.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lib/include/macro.h"

#define CACHE_LINE_SFT 6
#define CACHE_LINE_SZ (1 << CACHE_LINE_SFT)

/*
API

1. void * alloc_page(int p)
  Allocates 2^p contiguous physical pages and return the start address as a void
pointer. If it is not possible to allocate, return NULL.

2. void free_page(void * ptr)
  Deallocates the allocation made for `ptr`. The size of the allocation for ptr
will be stored by pgmgr.

Requirements
1. Allocate all objects at initialization
2. The two API are real-time
*/

/*
Reasoning

#---------------#...
#-------#-------#...
#---#---#---#---#...
#-#-#-#-#-#-#-#-#...

Always need some kind of quick lookup in the allocated pages for
`free_page(ptr)` call.

All links / data structures needs to be allocated in some fields embedded in the
page descriptor.

Is it possible to have a dynamic data structure for quick look up?
  Balanced tree (involves amortization)
  Hash table (needs fixed bucket size)
  Array (continuous address for a predetermined indexing strategy)

Why not link all of the page descriptors like this?
#---------------#...
#-------#-------#...
#---#---#---#---#...
#-#-#-#-#-#-#-#-#...
*/

#define PGDSP_USED (1 << 0)

// 2MB / 4KB = 512 pages
// 512 * 64 = 32KB continuous memory to describe a 2MB page
// less than 2% of total memory

// 64 bytes
// 64 bytes / 4KB = 1/64
// So about 4% of total memory will be used by the pgmgr to provide dynamic page
// allocation.
struct PgDsp
{
  uintptr_t addr;               // the start addr of the page
  struct PgDsp *free_list_next; // used when it is entirely free
  struct PgDsp *free_list_prev; // used when it is entirely free
  uint16_t flags;               //
  uint8_t lvl;                  // record how large is the allocation, needed by free_page
  uint8_t _;
};

/*
Explanation: max_combine_lvl

Page 0b1101
Lvl0 buddy 0b1100
Don't have Lvl1 buddy (because Lvl1 buddy only exists for page number mod 2 ==
0)

In a segment of 0b10000 - 0b10100
Page 0b10000
Lvl1 buddy 0b10001
Lvl2 buddy 0b10010
Lvl3 buddy 0b10100 <- outside the root segment
*/

struct PgRoot
{
  struct PgDsp *root; // first PgDsp in this segment
  uintptr_t start_addr;
  uintptr_t end_addr;
};

struct PgDList
{
  struct PgDsp *head;
};

/*
PG_SZ should be between [ 512 Byte, 2 MB ]
*/
// 512 Bytes = 2^9
// 4KB = 2^12
// 2MB = 2^9 * 4KB = 2^21
#define PG_SFT 20 // 1MB
#define PG_SZ (1 << PG_SFT)
#define PGMGR_MAXLVL 9

struct PgMgr
{
  // premature optimization is the root of all evil
  // Support contiguous page upto 2MB
  struct PgDList free_lists[PGMGR_MAXLVL + 1];
  unsigned roots_n;      // How large can it be? Each root is 2MB. For 8GB, there are
                         // 4000 entries. Not a lot!
  struct PgRoot roots[]; // binary search for free_page
};

/********************** API **********************/

void *pg_alloc_page(struct PgMgr *mgr, uint8_t p, uint16_t flags);

void pg_free_page(struct PgMgr *mgr, void *addr);

/********************** DEBUG **********************/

void pgmgr_debug_print(const struct PgMgr *mgr, bool verbose);

struct PgMgrDebug
{
  struct PgMgr *mgr;
  uint32_t
      roots_n; // passed in as what are the capacity in the roots stat arrays
  uintptr_t *roots_num_pages;
  uintptr_t *roots_num_pages_used;
  uint32_t free_lists_cnt[PGMGR_MAXLVL + 1];
};

void pgmgr_debug(const struct PgMgr *mgr, struct PgMgrDebug *debug);

#endif
