#ifndef L_ELF_READER_H
#define L_ELF_READER_H

#include "elf.h"

struct ElfReader
{
  const Elf64_Ehdr * e_hdr;

  /* Section Headers */
  const Elf64_Shdr * s_hdr_arr;
  Elf64_Half s_hdr_arrlen;

  const Elf64_Shdr * s_hdr_hash;
  const Elf64_Shdr * s_hdr_dynsym;
  const Elf64_Shdr * s_hdr_dynstr;
  const Elf64_Shdr * s_hdr_rela_dyn;
  const Elf64_Shdr * s_hdr_text;
  const Elf64_Shdr * s_hdr_rodata;
  const Elf64_Shdr * s_hdr_data_rel_ro;
  const Elf64_Shdr * s_hdr_dynamic;
  const Elf64_Shdr * s_hdr_got;
  const Elf64_Shdr * s_hdr_got_plt;
  const Elf64_Shdr * s_hdr_comment;
  const Elf64_Shdr * s_hdr_symtab;
  const Elf64_Shdr * s_hdr_strtab;
  const Elf64_Shdr * s_hdr_shstrtab;

  /* Program Headers */
  const Elf64_Phdr * p_hdr_arr;
  Elf64_Half p_hdr_arrlen;

  /* Dynamic Sections */
  const Elf64_Dyn * dyn_arr;
  Elf64_Xword dyn_arrlen;

  /* Relocations */
  const Elf64_Rela * rela_arr;
  Elf64_Xword rela_arrlen;
};

/* Public Macro Helper */

#define ELF_BASE_CHAR_PTR(elf_reader) ( (const char *) (elf_reader)->e_hdr )

/* Public Function */

struct LoaderArgs; // Forward Declaration
struct ElfReader elf_reader_ctr(const struct LoaderArgs * loader_args);

#endif