#include "elf_reader.h"
#include "loader_priv.h"
#include "lib/include/util.h"

/* Macro Helper */

#define ELF_SHSTRTAB_GET(elf_reader, idx) ( ELF_BASE_CHAR_PTR(elf_reader) + ((elf_reader)->s_hdr_shstrtab->sh_offset + (idx)) )
#define ELF_STRTAB_GET(elf_reader, idx) ( ELF_BASE_CHAR_PTR(elf_reader) + ((elf_reader)->s_hdr_strtab->sh_offset + (idx)) )
#define ELF_SHDR_GET(elf_reader, idx) ( &(elf_reader)->s_hdr_arr[idx] )

/* Static Helper Functions */

static bool elf_reader_check_e_ident(const struct ElfReader * elf_reader)
{
  const unsigned char * e_ident = elf_reader->e_hdr->e_ident;
  bool check = e_ident[0] == 0x7f && e_ident[1] == 'E' && e_ident[2] == 'L' && e_ident[3] == 'F' // Default
               && e_ident[4] == 2                                                                // ELFCLASS64
               && e_ident[5] == 1                                                                // ELFDATA2LSB
               && e_ident[6] == 1;                                                               // EV_CURRENT
  if (!check)
  {
    DEBUG_LOADER_PRINTF("e_ident is ");
    for (int i = 0; i < 7; i += 1)
    {
      DEBUG_LOADER_PRINTF("0x%x ", e_ident[i]);
    }
  }
  return check;
}

static void print_section_permission(const Elf64_Shdr *hdr)
{
  DEBUG_LOADER_PRINTF("Permission ");
  if (hdr->sh_flags & 0x1)
  {
    DEBUG_LOADER_PRINTF("W");
  }
  if (hdr->sh_flags & 0x2)
  {
    DEBUG_LOADER_PRINTF("A");
  }
  if (hdr->sh_flags & 0x4)
  {
    DEBUG_LOADER_PRINTF("E");
  }
  DEBUG_LOADER_PRINTF("\r\n");
}

static void print_section_progbits(const Elf64_Shdr *hdr)
{
  print_section_permission(hdr);
  DEBUG_LOADER_PRINTF("sh_addr 0x%llx\r\n", hdr->sh_addr);
  DEBUG_LOADER_PRINTF("sh_offset 0x%llx\r\n", hdr->sh_offset);
  DEBUG_LOADER_PRINTF("sh_size 0x%llx\r\n", hdr->sh_size);
  DEBUG_LOADER_PRINTF("sh_addralign 0x%llx\r\n", hdr->sh_addralign);
}

static void print_section_symtab(const struct ElfReader *elf_reader)
{
  const Elf64_Shdr *s_hdr_symtab = elf_reader->s_hdr_symtab;

  print_section_permission(s_hdr_symtab);

  const Elf64_Shdr *s_hdr_strtab = ELF_SHDR_GET(elf_reader, s_hdr_symtab->sh_link);
  const char *strtab = ELF_BASE_CHAR_PTR(elf_reader) + s_hdr_strtab->sh_offset;

  const Elf64_Sym *sym_arr = (const Elf64_Sym *)(ELF_BASE_CHAR_PTR(elf_reader) + s_hdr_symtab->sh_offset);

  DEBUG_LOADER_PRINTF("Number of Local Symbols (sh_info) = %d\r\n", s_hdr_symtab->sh_info);

  int num_entries = s_hdr_symtab->sh_size / s_hdr_symtab->sh_entsize;
  for (int i = 0; i < num_entries; i += 1)
  {
    const Elf64_Sym *symbol = &sym_arr[i];
    DEBUG_LOADER_PRINTF("Symbol %d: %s\t", i, &strtab[symbol->st_name]);

    switch (symbol->st_info >> 4)
    {
    case STB_LOCAL:
      DEBUG_LOADER_PRINTF("STB_LOCAL");
      break;
    case STB_GLOBAL:
      DEBUG_LOADER_PRINTF("STB_GLOBAL");
      break;
    case STB_WEAK:
      DEBUG_LOADER_PRINTF("STB_WEAK");
      break;
    default:
      DEBUG_LOADER_PRINTF("!%d!", symbol->st_info);
      break;
    }
    DEBUG_LOADER_PRINTF("\t");

    switch (symbol->st_shndx)
    {
    case SHN_UNDEF:
      DEBUG_LOADER_PRINTF("SHN_UNDEF");
      break;
    case SHN_COMMON:
      DEBUG_LOADER_PRINTF("SHN_COMMON");
      break;
    case SHN_ABS:
      DEBUG_LOADER_PRINTF("SHN_ABS");
      break;
    default:
      DEBUG_LOADER_PRINTF("!%d!", symbol->st_shndx);
      break;
    }
    DEBUG_LOADER_PRINTF("\t");

    DEBUG_LOADER_PRINTF("0x%llx\r\n", symbol->st_value);
  }
}

static void elf_reader_ctr_section(struct ElfReader * elf_reader, const Elf64_Shdr *s_hdr)
{
  const char * sh_name = ELF_SHSTRTAB_GET(elf_reader, s_hdr->sh_name);

  DEBUG_LOADER_PRINTF("==== ELF Section ====\r\n");

  switch (s_hdr->sh_type)
  {
  case SHT_NULL:
    DEBUG_LOADER_PRINTF("SHT_NULL\r\n");
    break;

  case SHT_STRTAB:
    DEBUG_LOADER_PRINTF("SHT_STRTAB %s\r\n", sh_name);
    if (util_strcmp(sh_name, ".strtab"))
      elf_reader->s_hdr_strtab = s_hdr;
    if (util_strcmp(sh_name, ".dynstr"))
      elf_reader->s_hdr_dynstr = s_hdr;
    break;

  case SHT_PROGBITS:
    DEBUG_LOADER_PRINTF("SHT_PROGBITS %s\r\n", sh_name);
    print_section_progbits(s_hdr);
    if (util_strcmp(sh_name, ".text"))
      elf_reader->s_hdr_text = s_hdr;
    if (util_strcmp(sh_name, ".rodata"))
      elf_reader->s_hdr_rodata = s_hdr;
    if (util_strcmp(sh_name, ".data.rel.ro"))
      elf_reader->s_hdr_data_rel_ro = s_hdr;
    if (util_strcmp(sh_name, ".comment"))
      elf_reader->s_hdr_comment = s_hdr;
    if (util_strcmp(sh_name, ".got"))
      elf_reader->s_hdr_got = s_hdr;
    if (util_strcmp(sh_name, ".got.plt"))
      elf_reader->s_hdr_got_plt = s_hdr;
    break;

  case SHT_SYMTAB:
    DEBUG_LOADER_PRINTF("SHT_SYMTAB %s\r\n", sh_name);
    if (util_strcmp(sh_name, ".symtab"))
      elf_reader->s_hdr_symtab = s_hdr;
    break;

  case SHT_RELA:
    DEBUG_LOADER_PRINTF("SHT_RELA %s\r\n", sh_name);
    if (util_strcmp(sh_name, ".rela.dyn"))
      elf_reader->s_hdr_rela_dyn = s_hdr;
    break;

  case SHT_HASH:
    DEBUG_LOADER_PRINTF("SHT_HASH %s\r\n", sh_name);
    if (util_strcmp(sh_name, ".hash"))
      elf_reader->s_hdr_hash = s_hdr;
    break;

  case SHT_DYNAMIC:
    DEBUG_LOADER_PRINTF("SHT_DYNAMIC %s\r\n", sh_name);
    if (util_strcmp(sh_name, ".dynamic"))
      elf_reader->s_hdr_dynamic = s_hdr;
    break;

  case SHT_NOBITS:
    DEBUG_LOADER_PRINTF("SHT_NOBITS %s\r\n", sh_name);
    elf_reader->s_hdr_bss = s_hdr;

  case SHT_REL:
    DEBUG_LOADER_PRINTF("SHT_REL %s\r\n", sh_name);
    break;

  case SHT_DYNSYM:
    DEBUG_LOADER_PRINTF("SHT_DYNSYM %s\r\n", sh_name);
    if (util_strcmp(sh_name, ".dynsym"))
      elf_reader->s_hdr_dynsym = s_hdr;
    break;
  }
}

static void elf_reader_ctr_sections(struct ElfReader * elf_reader)
{
  const Elf64_Ehdr * e_hdr = elf_reader->e_hdr;
  // Set the shstrtab
  elf_reader->s_hdr_shstrtab = ELF_SHDR_GET(elf_reader, e_hdr->e_shstrndx);

  for (int i = 0; i < elf_reader->s_hdr_arrlen; i += 1)
  {
    elf_reader_ctr_section(elf_reader, &elf_reader->s_hdr_arr[i]);
  }
}

static void print_ehdr(const struct ElfReader * elf_reader)
{
  const Elf64_Ehdr * hdr = elf_reader->e_hdr;
  DEBUG_LOADER_PRINTF("e_type %d\r\n", hdr->e_type);
  DEBUG_LOADER_PRINTF("e_machine %d\r\n", hdr->e_machine);
  DEBUG_LOADER_PRINTF("e_version %d\r\n", hdr->e_version);
  DEBUG_LOADER_PRINTF("e_entry 0x%llx\r\n", hdr->e_entry);
  DEBUG_LOADER_PRINTF("e_phoff 0x%llx\r\n", hdr->e_phoff);
  DEBUG_LOADER_PRINTF("e_shoff 0x%llx\r\n", hdr->e_shoff);
  DEBUG_LOADER_PRINTF("e_flags %d\r\n", hdr->e_flags);
  DEBUG_LOADER_PRINTF("e_ehsize %d\r\n", hdr->e_ehsize);
  DEBUG_LOADER_PRINTF("e_phentsize %d\r\n", hdr->e_phentsize);
  DEBUG_LOADER_PRINTF("e_phnum %d\r\n", hdr->e_phnum);
  DEBUG_LOADER_PRINTF("e_shentsize %d\r\n", hdr->e_shentsize);
  DEBUG_LOADER_PRINTF("e_shnum %d\r\n", hdr->e_shnum);
  DEBUG_LOADER_PRINTF("e_shstrndx %d\r\n", hdr->e_shstrndx);
}

static void print_phdr(const Elf64_Phdr *hdr)
{
  DEBUG_LOADER_PRINTF("==== PROGRAM HEADER ====\r\n");
  switch (hdr->p_type)
  {
  case PT_NULL:
    DEBUG_LOADER_PRINTF("PT_NULL ");
    break;
  case PT_LOAD:
    DEBUG_LOADER_PRINTF("PT_LOAD ");
    break;
  case PT_DYNAMIC:
    DEBUG_LOADER_PRINTF("PT_DYNAMIC ");
    break;
  case PT_INTERP:
    DEBUG_LOADER_PRINTF("PT_INTERP ");
    break;
  case PT_PHDR:
    DEBUG_LOADER_PRINTF("PT_PHDR ");
    break;
  case PT_GNU_RELRO:
    DEBUG_LOADER_PRINTF("PT_GNU_RELRO ");
    break;
  default:
    DEBUG_LOADER_PRINTF("PT_UNKNOWN=%llx ", hdr->p_type);
    break;
  }
  if (hdr->p_flags & 0x1)
  {
    DEBUG_LOADER_PRINTF("X");
  }
  if (hdr->p_flags & 0x2)
  {
    DEBUG_LOADER_PRINTF("W");
  }
  if (hdr->p_flags & 0x4)
  {
    DEBUG_LOADER_PRINTF("R");
  }
  DEBUG_LOADER_PRINTF("\r\n");
  DEBUG_LOADER_PRINTF("p_offset 0x%llx\r\n", hdr->p_offset);
  DEBUG_LOADER_PRINTF("p_vaddr/p_paddr 0x%llx/0x%llx\r\n", hdr->p_vaddr, hdr->p_paddr);
  DEBUG_LOADER_PRINTF("p_filesz/p_memsz 0x%llx/0x%llx\r\n", hdr->p_filesz, hdr->p_memsz);
  DEBUG_LOADER_PRINTF("p_align 0x%llx\r\n", hdr->p_align);
}

static void elf_reader_parse_dynamic(struct ElfReader *elf_reader)
{
  const Elf64_Shdr * s_hdr_dynamic = elf_reader->s_hdr_dynamic;
  elf_reader->dyn_arrlen = s_hdr_dynamic->sh_size / s_hdr_dynamic->sh_entsize;
  elf_reader->dyn_arr = (const Elf64_Dyn *) (ELF_BASE_CHAR_PTR(elf_reader) + s_hdr_dynamic->sh_offset);
  
  Elf64_Xword rela_ent = 0, rela_sz = 0;
  for (int i = 0; i < (int) elf_reader->dyn_arrlen; i += 1) {
    const Elf64_Dyn * dyn_entry = &elf_reader->dyn_arr[i];
    switch (dyn_entry->d_tag)
    {
    case DT_RELA:
      elf_reader->rela_arr = (const Elf64_Rela *) (ELF_BASE_CHAR_PTR(elf_reader) + dyn_entry->d_un.d_ptr);
      break;

    case DT_RELAENT:
      rela_ent = dyn_entry->d_un.d_val;
      break;

    case DT_RELASZ:
      rela_sz = dyn_entry->d_un.d_val;
      break;

    default:
      break;
    }
  }

  elf_reader->rela_arrlen = rela_sz / rela_ent;
}

/* Public Functions */

struct ElfReader elf_reader_ctr(const struct LoaderArgs * loader_args)
{
  struct ElfReader elf_reader;
  util_memset(&elf_reader, 0x0, sizeof(elf_reader));

  elf_reader.e_hdr = (const Elf64_Ehdr *) loader_args->elf_start;

  if (!elf_reader_check_e_ident(&elf_reader)) {
    elf_reader.e_hdr = NULL;
    return elf_reader;
  }

  print_ehdr(&elf_reader);

  elf_reader.s_hdr_arr = (const Elf64_Shdr *) (ELF_BASE_CHAR_PTR(&elf_reader) + elf_reader.e_hdr->e_shoff);
  elf_reader.s_hdr_arrlen = elf_reader.e_hdr->e_shnum;

  elf_reader.p_hdr_arr = (const Elf64_Phdr *) (ELF_BASE_CHAR_PTR(&elf_reader) + elf_reader.e_hdr->e_phoff);
  elf_reader.p_hdr_arrlen = elf_reader.e_hdr->e_phnum;

  elf_reader_ctr_sections(&elf_reader);

  // print_section_symtab(&elf_reader);

  for (int i = 0; i < elf_reader.p_hdr_arrlen; i += 1) {
    print_phdr(&elf_reader.p_hdr_arr[i]);
  }

  elf_reader_parse_dynamic(&elf_reader);

  return elf_reader;
}
