#include "loader_priv.h"
#include "lib/include/printf.h"

void load_elf(const char *elf_start)
{
  bool p = true;

  printf("elf_start = %p\n", elf_start);
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
}