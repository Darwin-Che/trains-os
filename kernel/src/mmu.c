#include <stdint.h>
#include "lib/include/printf.h"
#include "mmu.h"

extern uint64_t id_pgd_addr();
extern void memzero(uint64_t addr, uint64_t sz);

void create_table_entry(uint64_t tbl, uint64_t tbl_next, uint64_t va, uint64_t shift, uint64_t flags)
{
  uint64_t tbl_idx = va >> shift;
  tbl_idx &= (ENTIRES_PER_TABLE - 1);
  uint64_t descriptor = tbl_next | flags;
  *((uint64_t *)(tbl + (tbl_idx << 3))) = descriptor;
}

void create_block_entry(uint64_t pmd, uint64_t vstart, uint64_t vend, uint64_t pa)
{
  vstart >>= SECTION_SHIFT;
  vstart &= (ENTIRES_PER_TABLE - 1);

  vend >>= SECTION_SHIFT;
  vend -= 1;
  vend &= (ENTIRES_PER_TABLE - 1);

  pa >>= SECTION_SHIFT;
  pa <<= SECTION_SHIFT;

  do
  {
    uint64_t _pa = pa;

    if (pa >= DEVICE_BASE)
    {
      _pa |= TD_DEVICE_BLOCK_FLAGS;
    }
    else if (pa < DATA_BASE)
    {
      _pa |= TD_CODE_BLOCK_FLAGS;
    }
    else
    {
      _pa |= TD_DATA_BLOCK_FLAGS;
    }

    *((uint64_t *)(pmd + (vstart << 3))) = _pa;
    pa += SECTION_SIZE;
    vstart += 1;
  } while (vstart <= vend);
}

void create_page_tables()
{
  uint64_t id_pgd = (uint64_t)id_pgd_addr();
  memzero(id_pgd, ID_MAP_TBL_SZ);

  uint64_t tbl = id_pgd;
  uint64_t tbl_next = tbl + PAGE_SIZE;

  create_table_entry(tbl, tbl_next, 0x0, PGD_SHIFT, TD_TABLE_FLAGS);

  tbl += PAGE_SIZE;
  tbl_next += PAGE_SIZE;

  for (uint64_t i = 0; i < 4; i += 1)
  {
    uint64_t offset = PUD_SLICE_SZ * i;

    create_table_entry(tbl, tbl_next, offset, PUD_SHIFT, TD_TABLE_FLAGS);

    create_block_entry(tbl_next, offset, offset + PUD_SLICE_SZ, offset);

    tbl_next += PAGE_SIZE;
  }
}

void set_page_tables()
{
  uint64_t *pg_ptr = (uint64_t *)id_pgd_addr();
  asm volatile("msr ttbr0_el1, %0"
               : "=r"(pg_ptr));
}

/***********/

void k_page_table_entry_print(uint64_t entry)
{
  if (entry & (0x1 << 0))
  {
    // valid
    if (entry & (0x1 << 1))
    {
      // table pointer
      uint64_t addr = entry & ((((uint64_t)1) << 48) - 1);
      addr = addr & ~((1 << 12) - 1);
      printf("To table at %X (%X)", addr, entry);
    }
    else
    {
      // block
      uint64_t addr = entry & ((((uint64_t)1) << 48) - 1);
      addr = addr & ~((1 << 12) - 1);
      printf("To block at %X (%X)", addr, entry);
    }
  }
  else
  {
    // invalid
    printf("Invalid Entry %X", entry);
  }
  printf("\r\n");
}

void k_page_table_print(uint64_t *table)
{
  printf("Page Table Dump %X\r\n", table);
  printf("------------\r\n");
  for (int i = 0; i < 5; i += 1)
  {
    printf("%X[%d] :: ", table, i);
    k_page_table_entry_print(table[i]);
  }
  printf(".....\r\n");
  for (int i = 512 - 5; i < 512; i += 1)
  {
    printf("%X[%d] :: ", table, i);
    k_page_table_entry_print(table[i]);
  }
  printf("------------\r\n\r\n");
}

void k_page_table_print_all()
{
  uint64_t tcr, mair;
  asm volatile("mrs %0, tcr_el1"
               : "=r"(tcr));
  printf("TCR_EL1 = %X\r\n", tcr);
  asm volatile("mrs %0, mair_el1"
               : "=r"(mair));
  printf("MAIR_EL1 = %X\r\n", mair);

  // pg_dir = 0x9a000;
  uint64_t *pg_ptr = (uint64_t *)id_pgd_addr();
  k_page_table_print(pg_ptr);
  k_page_table_print(pg_ptr + 512);
  k_page_table_print(pg_ptr + 512 * 2);
  k_page_table_print(pg_ptr + 512 * 3);
  k_page_table_print(pg_ptr + 512 * 4);
  k_page_table_print(pg_ptr + 512 * 5);
}
