#include "loader_priv.h"
#include "lib/include/printf.h"
#include "lib/include/macro.h"
#include "lib/include/uapi.h"
#include "lib/include/util.h"

typedef void (*EntryFunc)(uint64_t);

void calc_mem_size(const Elf64_Phdr *p_hdr_start, uint16_t hdr_cnt, uint64_t *vaddr_base, uint64_t *mem_sz);
void memcpy_prog_hdr(const Elf64_Phdr *p_hdr, const char *elf_start, uint64_t vaddr_diff);

void load_elf(const char *elf_start)
{
  bool p = true;

  printf("elf_start = %p\r\n", elf_start);
  const Elf64_Ehdr *e_hdr = (const Elf64_Ehdr *)elf_start;
  check_elf_hdr(p, e_hdr);

  // Check section headers
  const Elf64_Shdr *s_hdr = (const Elf64_Shdr *)(elf_start + e_hdr->e_shoff);
  for (int i = 0; i < e_hdr->e_shnum; i += 1)
  {
    check_section(p, &s_hdr[i], e_hdr);
  }

  // Print program headers
  const Elf64_Phdr *p_hdr = (const Elf64_Phdr *)(elf_start + e_hdr->e_phoff);
  for (int i = 0; i < e_hdr->e_phnum; i += 1)
  {
    if (p)
      print_prog_hdr(&p_hdr[i]);
  }

  // Load program headers
  // Step 1 : How much memory should we request for loading?
  //   This need 1) min v_addr, 2) max v_addr, 3) max align
  uint64_t vaddr_base, mem_sz;
  calc_mem_size(p_hdr, e_hdr->e_phnum, &vaddr_base, &mem_sz);

  printf("Step 1 : vaddr_base = %x, mem_sz = %x\r\n", vaddr_base, mem_sz);

  // Step 2 : Request memory with mem_sz, get vaddr_target and relo_offset
  void *vaddr_target;
  ke_mmap(&vaddr_target, mem_sz);

  uint64_t vaddr_diff = (uint64_t) vaddr_target - vaddr_base;
  printf("Step 2 : vaddr_target = %x, vaddr_diff = %x\r\n", vaddr_target, vaddr_diff);

  // Step 3 : memcpy all of the program header sections into the memory
  printf("Step 3 : MEMCPY\r\n");
  for (int i = 0; i < e_hdr->e_phnum; i += 1)
  {
    const Elf64_Phdr *h = &p_hdr[i];
    if (h->p_type != PT_LOAD)
      continue;
    memcpy_prog_hdr(&p_hdr[i], elf_start, vaddr_diff);
  }
  
  // Step 4 : apply any relocations

  // Step 5 : Jump to function start
  EntryFunc entry_func = (EntryFunc) (e_hdr->e_entry + vaddr_diff);
  printf("Step 5 : Jump to %x\r\n", entry_func);

  // DEBUG
  const uint32_t * peek_data = (const uint32_t *) entry_func;
  printf("%x\r\n", peek_data[0]);
  printf("%x\r\n", peek_data[1]);
  printf("%x\r\n", peek_data[2]);
  printf("%x\r\n", peek_data[3]);
  printf("%x\r\n", peek_data[4]);
  printf("%x\r\n", peek_data[5]);

  ke_print_raw(NULL);

  entry_func(0);
}

void memcpy_prog_hdr(const Elf64_Phdr *p_hdr, const char *elf_start, uint64_t vaddr_diff) {
  uint64_t dest = p_hdr->p_vaddr + vaddr_diff;
  const char *src = &elf_start[p_hdr->p_offset];
  printf("memcpy from %x to %x of size %x\r\n", src, dest, p_hdr->p_filesz);
  util_memcpy((void*) dest, src, p_hdr->p_filesz);
}

void calc_mem_size(const Elf64_Phdr *p_hdr_start, uint16_t hdr_cnt, uint64_t *vaddr_base, uint64_t *mem_sz)
{
  uint64_t min_addr = UINT64_MAX;
  uint64_t max_addr = 0;
  uint64_t max_align = 0x1;

  for (int i = 0; i < hdr_cnt; i += 1)
  {
    const Elf64_Phdr *hdr = &p_hdr_start[i];
    switch (hdr->p_type)
    {
    case PT_LOAD:
    case PT_GNU_RELRO:
      break;
    case PT_NULL:
    case PT_PHDR:
      continue;
    default:
      printf("PT_UNKNOWN=%d ", hdr->p_type);
      continue;
    }

    if (hdr->p_vaddr < min_addr)
      min_addr = hdr->p_vaddr;
    if (hdr->p_vaddr + hdr->p_memsz > max_addr)
      max_addr = hdr->p_vaddr + hdr->p_memsz;
    if (hdr->p_align > max_align)
      max_align = hdr->p_align;
  }

  // We assume that max_align is a perfect power of 2

  *vaddr_base = min_addr & ~(max_align - 1);

  // raw mem_sz

  *mem_sz = max_addr - *vaddr_base;
}