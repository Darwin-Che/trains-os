#ifndef K_MM_H
#define K_MM_H

// The bottom
#define DATA_BASE 0x00200000 // 2 * (2^20) = 2MB = exactly one PMD
#define DEVICE_BASE 0xFC000000

#define PAGE_SHIFT 12
#define TABLE_SHIFT 9
#define SECTION_SHIFT (PAGE_SHIFT + TABLE_SHIFT)

#define PAGE_SIZE (1 << PAGE_SHIFT)
#define SECTION_SIZE (1 << SECTION_SHIFT)

#define PGD_SHIFT (PAGE_SHIFT + 3 * TABLE_SHIFT)
#define PUD_SHIFT (PAGE_SHIFT + 2 * TABLE_SHIFT)
#define PMD_SHIFT (PAGE_SHIFT + TABLE_SHIFT)

/********/

#define ENTIRES_PER_TABLE (1 << TABLE_SHIFT)

#define PUD_SLICE_SZ (1 << PUD_SHIFT)

#define ID_MAP_TBL_SZ (6 * PAGE_SIZE)

/*
 * Memory region attributes:
 *
 *   n = AttrIndx[2:0]
 *			n	MAIR
 *   DEVICE_nGnRnE	000	00000000
 *   NORMAL_NC		  001	01000100
 *   NORMAL_CACHE   010 01001111
 *   ALL_CACHE      011 11111111
 */
#define MATTR_DEVICE_nGnRnE 0x0
#define MATTR_NORMAL_NC 0x44
#define MATTR_NORMAL_CACHE 0x4F
#define MATTR_ALL_CACHE 0xFF
#define MATTR_DEVICE_nGnRnE_INDEX 0
#define MATTR_NORMAL_NC_INDEX 1
#define MATTR_NORMAL_CACHE_INDEX 2
#define MATTR_ALL_CACHE_INDEX 3
#define MAIR_VALUE (                                         \
    (MATTR_NORMAL_NC << (8 * MATTR_NORMAL_NC_INDEX)) |       \
    MATTR_DEVICE_nGnRnE << (8 * MATTR_DEVICE_nGnRnE_INDEX) | \
    MATTR_NORMAL_CACHE << (8 * MATTR_NORMAL_CACHE_INDEX) |   \
    MATTR_ALL_CACHE << (8 * MATTR_ALL_CACHE_INDEX))

/********/

#define TD_VALID (1 << 0)
#define TD_BLOCK (0 << 1)
#define TD_TABLE (1 << 1)
#define TD_ACCESS (1 << 10)
#define TD_DATA_PERMS (1 << 6)
#define TD_CODE_PERMS (0 << 6)
#define TD_INNER_SHARABLE (3 << 8)
// #define TD_NON_SHARABLE (0 << 8)

#define TD_TABLE_FLAGS (TD_TABLE | TD_VALID)
#define TD_DATA_BLOCK_FLAGS (TD_ACCESS | TD_INNER_SHARABLE | TD_DATA_PERMS | (MATTR_ALL_CACHE_INDEX << 2) | TD_BLOCK | TD_VALID)
#define TD_DEVICE_BLOCK_FLAGS (TD_ACCESS | TD_INNER_SHARABLE | TD_DATA_PERMS | (MATTR_DEVICE_nGnRnE_INDEX << 2) | TD_BLOCK | TD_VALID)
#define TD_CODE_BLOCK_FLAGS (TD_ACCESS | TD_INNER_SHARABLE | TD_CODE_PERMS | (MATTR_ALL_CACHE_INDEX << 2) | TD_BLOCK | TD_VALID)

// void create_page_tables();
// void set_page_tables();

#endif