#ifndef L_ELF_H
#define L_ELF_H

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

typedef struct
{
  unsigned char e_ident[16]; /* ELF identification */
  Elf64_Half e_type;         /* Object file type */
  Elf64_Half e_machine;      /* Machine type */
  Elf64_Word e_version;      /* Object file version */
  Elf64_Addr e_entry;        /* Entry point address */
  Elf64_Off e_phoff;         /* Program header offset */
  Elf64_Off e_shoff;         /* Section header offset */
  Elf64_Word e_flags;        /* Processor-specific flags */
  Elf64_Half e_ehsize;       /* ELF header size */
  Elf64_Half e_phentsize;    /* Size of program header entry */
  Elf64_Half e_phnum;        /* Number of program header entries */
  Elf64_Half e_shentsize;    /* Size of section header entry */
  Elf64_Half e_shnum;        /* Number of section header entries */
  Elf64_Half e_shstrndx;     /* Section name string table index */
} Elf64_Ehdr;

typedef struct
{
  Elf64_Word sh_name;       /* Section name */
  Elf64_Word sh_type;       /* Section type */
  Elf64_Xword sh_flags;     /* Section attributes */
  Elf64_Addr sh_addr;       /* Virtual address in memory */
  Elf64_Off sh_offset;      /* Offset in file */
  Elf64_Xword sh_size;      /* Size of section */
  Elf64_Word sh_link;       /* Link to other section */
  Elf64_Word sh_info;       /* Miscellaneous information */
  Elf64_Xword sh_addralign; /* Address alignment boundary */
  Elf64_Xword sh_entsize;   /* Size of entries, if section has table */
} Elf64_Shdr;

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_REL 9
#define SHT_DYNSYM 11

typedef struct
{
  Elf64_Word p_type;    /* Type of segment */
  Elf64_Word p_flags;   /* Segment attributes */
  Elf64_Off p_offset;   /* Offset in file */
  Elf64_Addr p_vaddr;   /* Virtual address in memory */
  Elf64_Addr p_paddr;   /* Reserved */
  Elf64_Xword p_filesz; /* Size of segment in file */
  Elf64_Xword p_memsz;  /* Size of segment in memory */
  Elf64_Xword p_align;  /* Alignment of segment */
} Elf64_Phdr;

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_GNU_RELRO 0x6474E552

typedef struct
{
  Elf64_Word st_name;     /* Symbol name */
  unsigned char st_info;  /* Type and Binding attributes */
  unsigned char st_other; /* Reserved */
  Elf64_Half st_shndx;    /* Section table index */
  Elf64_Addr st_value;    /* Symbol value */
  Elf64_Xword st_size;    /* Size of object (e.g., common) */
} Elf64_Sym;

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2

#define SHN_UNDEF 0
#define SHN_ABS 0xFFF1
#define SHN_COMMON 0xFFF2

typedef struct {
  Elf64_Xword d_tag;
  union {
    Elf64_Xword d_val;
    Elf64_Addr d_ptr;
  } d_un;
} Elf64_Dyn;

#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9

typedef struct {
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
  Elf64_Sxword r_addend;
} Elf64_Rela;

#define ELF64_R_SYM(info)             ((info) >> 32)
#define ELF64_R_TYPE(info)            ((Elf64_Word)(info))
#define ELF64_R_INFO(sym, type)       (((Elf64_Xword)(sym) << 32) + (Elf64_Xword)(type))

#define R_AARCH64_RELATIV 1027

#endif