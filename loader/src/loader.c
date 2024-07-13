#include "loader_priv.h"
#include "elf_reader.h"
#include "lib/include/printf.h"
#include "lib/include/macro.h"
#include "lib/include/uapi.h"
#include "lib/include/util.h"

typedef void (*EntryFunc)(const char *args, uint64_t args_len);

/* Static Helper Functions */

static void apply_relocation_relativ(const struct ElfReader *elf_reader, const Elf64_Rela * rela, int64_t vaddr_diff) {
  (void) elf_reader;
  Elf64_Sxword addend = rela->r_addend;
  Elf64_Addr offset = rela->r_offset;

  int64_t * target = (int64_t *) (offset + vaddr_diff);
  int64_t val = addend + vaddr_diff;

  // DEBUG_LOADER_PRINTF("R_AARCH64_RELATIV 0x%p = 0x%x\r\n", target, val);

  *target = val;
}

static void apply_relocations(const struct ElfReader *elf_reader, int64_t vaddr_diff) {
  for (Elf64_Xword i = 0; i < elf_reader->rela_arrlen; i += 1)
  {
    const Elf64_Rela * rela = &elf_reader->rela_arr[i];
    switch (ELF64_R_TYPE(rela->r_info))
    {
    case R_AARCH64_RELATIV:
      apply_relocation_relativ(elf_reader, rela, vaddr_diff);
      break;
    
    default:
      DEBUG_LOADER_PRINTF("Relocation %d is not supported\r\n", ELF64_R_TYPE(rela->r_info));
      break;
    }
  }
}

static void memcpy_prog_hdr(const struct ElfReader *elf_reader, int64_t vaddr_diff) {
  for (Elf64_Half i = 0; i < elf_reader->p_hdr_arrlen; i += 1)
  {
    const Elf64_Phdr *p_hdr = &elf_reader->p_hdr_arr[i];
    if (p_hdr->p_type != PT_LOAD)
      continue;

    uint64_t dest = p_hdr->p_vaddr + vaddr_diff;
    const char *src = ELF_BASE_CHAR_PTR(elf_reader) + p_hdr->p_offset;
    DEBUG_LOADER_PRINTF("memcpy from %x to %x of size %x\r\n", src, dest, p_hdr->p_filesz);
    util_memcpy((void*) dest, src, p_hdr->p_filesz);
  }
}

static void calc_mem_size(const struct ElfReader *elf_reader, uint64_t *vaddr_base, uint64_t *memsz)
{
  uint64_t min_addr = UINT64_MAX;
  uint64_t max_addr = 0;
  uint64_t max_align = 0x1;

  for (Elf64_Half i = 0; i < elf_reader->p_hdr_arrlen; i += 1)
  {
    const Elf64_Phdr *hdr = &elf_reader->p_hdr_arr[i];
    switch (hdr->p_type)
    {
    case PT_LOAD:
    case PT_GNU_RELRO:
      break;
    case PT_NULL:
    case PT_PHDR:
      continue;
    default:
      DEBUG_LOADER_PRINTF("PT_UNKNOWN=%d ", hdr->p_type);
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

  // raw memsz

  *memsz = max_addr - *vaddr_base;
}

static void clear_bss(const struct ElfReader *elf_reader, int64_t vaddr_diff)
{
  if (elf_reader->s_hdr_bss != NULL) {
    uint64_t dest = elf_reader->s_hdr_bss->sh_addr + vaddr_diff;
    util_memset((void*) dest, 0x0, elf_reader->s_hdr_bss->sh_size);
  }
}

/* Public Functions */

void load_elf(const char *args, size_t args_len)
{
  DEBUG_LOADER_PRINTF("args = %p, args_len = %llu\r\n", args, args_len);

  for (uint64_t i = 0; i < args_len; i += 1) {
    if (args[i] == '\0') {
      DEBUG_LOADER_PRINTF(",");
    }
    DEBUG_LOADER_PRINTF("%c", args[i]);
  }
  DEBUG_LOADER_PRINTF("\r\nargs_len = %d\r\n", args_len);

  struct LoaderArgs loader_args = resolve_args(args, args_len);
  struct ElfReader elf_reader = elf_reader_ctr(&loader_args);

  DEBUG_LOADER_PRINTF("e_hdr = %p\r\n", elf_reader.e_hdr);

  if (elf_reader.e_hdr == NULL) {
    return;
  }

  // Load program headers
  // Step 1 : How much memory should we request for loading?
  //   This need 1) min v_addr, 2) max v_addr, 3) max align
  uint64_t vaddr_base, memsz;
  calc_mem_size(&elf_reader, &vaddr_base, &memsz);

  DEBUG_LOADER_PRINTF("Step 1 : vaddr_base = %x, memsz = %x\r\n", vaddr_base, memsz);

  // Step 2 : Request memory with memsz, get vaddr_target and relo_offset
  void *vaddr_target;
  ke_mmap(&vaddr_target, memsz);

  int64_t vaddr_diff = (int64_t) vaddr_target - (int64_t) vaddr_base;
  DEBUG_LOADER_PRINTF("Step 2 : vaddr_target = %x, vaddr_diff = %x\r\n", vaddr_target, vaddr_diff);

  // Step 3 : memcpy all of the program header sections into the memory
  DEBUG_LOADER_PRINTF("Step 3 : MEMCPY\r\n");
  memcpy_prog_hdr(&elf_reader, vaddr_diff);

  // Step 3.1 : clear bss section
  DEBUG_LOADER_PRINTF("Step 3.1 : Clear Bss\r\n");
  clear_bss(&elf_reader, vaddr_diff);
  
  // Step 4 : apply any relocations
  DEBUG_LOADER_PRINTF("Step 4 : Relocations\r\n");
  apply_relocations(&elf_reader, vaddr_diff);

  // Step 5 : Jump to function start
  EntryFunc entry_func = (EntryFunc) (elf_reader.e_hdr->e_entry + vaddr_diff);
  DEBUG_LOADER_PRINTF("LOADER : Jump to %x\r\n", entry_func);

  // DEBUG
  // const uint32_t * peek_data = (const uint32_t *) entry_func;
  // printf("%x\r\n", peek_data[0]);
  // printf("%x\r\n", peek_data[1]);
  // printf("%x\r\n", peek_data[2]);
  // printf("%x\r\n", peek_data[3]);
  // printf("%x\r\n", peek_data[4]);
  // printf("%x\r\n", peek_data[5]);

  entry_func(args, args_len);

  ke_exit();
}
