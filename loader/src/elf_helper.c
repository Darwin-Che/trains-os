#include "loader_priv.h"
#include "lib/include/printf.h"

bool check_elf_hdr_e_ident(bool p, const unsigned char *e_ident)
{
  bool check = e_ident[0] == 0x7f && e_ident[1] == 'E' && e_ident[2] == 'L' && e_ident[3] == 'F' // Default
               && e_ident[4] == 2                                                                // ELFCLASS64
               && e_ident[5] == 1                                                                // ELFDATA2LSB
               && e_ident[6] == 1;                                                               // EV_CURRENT
  if (p && !check)
  {
    printf("e_ident is ");
    for (int i = 0; i < 7; i += 1)
    {
      printf("0x%x ", e_ident[i]);
    }
  }
  return check;
}

bool check_elf_hdr(bool p, const Elf64_Ehdr *hdr)
{
  bool check = check_elf_hdr_e_ident(p, hdr->e_ident);
  if (p)
  {
    printf("e_type %d\n", hdr->e_type);
    printf("e_machine %d\n", hdr->e_machine);
    printf("e_version %d\n", hdr->e_version);
    printf("e_entry 0x%llx\n", hdr->e_entry);
    printf("e_phoff 0x%llx\n", hdr->e_phoff);
    printf("e_shoff 0x%llx\n", hdr->e_shoff);
    printf("e_flags %d\n", hdr->e_flags);
    printf("e_ehsize %d\n", hdr->e_ehsize);
    printf("e_phentsize %d\n", hdr->e_phentsize);
    printf("e_phnum %d\n", hdr->e_phnum);
    printf("e_shentsize %d\n", hdr->e_shentsize);
    printf("e_shnum %d\n", hdr->e_shnum);
    printf("e_shstrndx %d\n", hdr->e_shstrndx);
  }
  return check;
}

void print_section_permission(const Elf64_Shdr *hdr)
{
  printf("Permission ");
  if (hdr->sh_flags & 0x1)
  {
    printf("W");
  }
  if (hdr->sh_flags & 0x2)
  {
    printf("A");
  }
  if (hdr->sh_flags & 0x4)
  {
    printf("E");
  }
  printf("\n");
}

void print_section_progbits(const Elf64_Shdr *hdr)
{
  print_section_permission(hdr);
  printf("sh_addr 0x%llx\n", hdr->sh_addr);
  printf("sh_offset 0x%llx\n", hdr->sh_offset);
  printf("sh_size 0x%llx\n", hdr->sh_size);
  printf("sh_addralign 0x%llx\n", hdr->sh_addralign);
}

void print_section_symtab(const Elf64_Shdr *hdr, const Elf64_Ehdr *e_hdr)
{
  print_section_permission(hdr);
  const char *memblock = (const char *)e_hdr;
  const Elf64_Shdr *shstr_hdr = (const Elf64_Shdr *)(memblock + (e_hdr->e_shoff + hdr->sh_link * e_hdr->e_shentsize));
  const char *shstr = memblock + shstr_hdr->sh_offset;
  const Elf64_Sym *sym_arr = (const Elf64_Sym *)(memblock + hdr->sh_offset);

  printf("Number of Local Symbols (sh_info) = %d\n", hdr->sh_info);

  int num_entries = hdr->sh_size / hdr->sh_entsize;
  for (int i = 0; i < num_entries; i += 1)
  {
    const Elf64_Sym *symbol = &sym_arr[i];
    printf("Symbol %d: %s\t", i, &shstr[symbol->st_name]);

    switch (symbol->st_info >> 4)
    {
    case STB_LOCAL:
      printf("STB_LOCAL");
      break;
    case STB_GLOBAL:
      printf("STB_GLOBAL");
      break;
    case STB_WEAK:
      printf("STB_WEAK");
      break;
    default:
      printf("!%d!", symbol->st_info);
      break;
    }
    printf("\t");

    switch (symbol->st_shndx)
    {
    case SHN_UNDEF:
      printf("SHN_UNDEF");
      break;
    case SHN_COMMON:
      printf("SHN_COMMON");
      break;
    case SHN_ABS:
      printf("SHN_ABS");
      break;
    default:
      printf("!%d!", symbol->st_shndx);
      break;
    }
    printf("\t");

    printf("0x%llx\n", symbol->st_value);
  }
}

bool check_section(bool p, const Elf64_Shdr *hdr, const Elf64_Ehdr *e_hdr)
{
  const char *memblock = (const char *)e_hdr;
  const Elf64_Shdr *shstr_hdr = (const Elf64_Shdr *)(memblock + (e_hdr->e_shoff + e_hdr->e_shstrndx * e_hdr->e_shentsize));
  const char *shstr = memblock + shstr_hdr->sh_offset;

  if (p)
    printf("===== Section Header =====\n");

  switch (hdr->sh_type)
  {
  case SHT_NULL:
    if (p)
      printf("SHT_NULL\n");
    return true;

  case SHT_STRTAB:
    if (p)
      printf("SHT_STRTAB %s\n", &shstr[hdr->sh_name]);
    return true;

  case SHT_PROGBITS:
    if (p)
    {
      printf("SHT_PROGBITS %s\n", &shstr[hdr->sh_name]);
      print_section_progbits(hdr);
    }
    return true;

  case SHT_SYMTAB:
    if (p)
    {
      printf("SHT_SYMTAB %s\n", &shstr[hdr->sh_name]);
      print_section_symtab(hdr, e_hdr);
    }
    return true;

  default:
    return false;
  }
}

void print_prog_hdr(const Elf64_Phdr *hdr)
{
  bool ret_code = true;
  printf("==== PROGRAM HEADER ====\n");
  switch (hdr->p_type)
  {
  case PT_NULL:
    printf("PT_NULL ");
    break;
  case PT_LOAD:
    printf("PT_LOAD ");
    break;
  case PT_PHDR:
    printf("PT_PHDR ");
    break;
  default:
    printf("PT_UNKNOWN=%d ", hdr->p_type);
    ret_code = false;
  }
  if (hdr->p_flags & 0x1)
  {
    printf("X");
  }
  if (hdr->p_flags & 0x2)
  {
    printf("W");
  }
  if (hdr->p_flags & 0x4)
  {
    printf("R");
  }
  printf("\n");
  printf("p_offset 0x%llx\n", hdr->p_offset);
  printf("p_vaddr/p_paddr 0x%llx/0x%llx\n", hdr->p_vaddr, hdr->p_paddr);
  printf("p_filesz/p_memsz 0x%llx/0x%llx\n", hdr->p_filesz, hdr->p_memsz);
  printf("p_align 0x%llx\n", hdr->p_align);
}