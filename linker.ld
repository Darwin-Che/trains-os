ENTRY(_start)           /* ELF entry symbol - no strictly needed */

SECTIONS {
  /* Page Tabl */
  /* Arbitrary, aligned with 4KB (0x1000) */
  /* 2^18 = 256KB */
  . = 0x40000;
  PROVIDE(LINKER_CODE_START = .);
  id_pgd = .;
	.pd_pgd : { . += (6 * (1 << 12)); }

  /* Kernel Segment */
  /* 2^19 = 512KB */
  . = 0x80000;
	.text : {
    KEEP("build/kernel.o"(.text.boot))
    build/kernel.o (.text*)
    build/loader.o (.text*)
	}

  /* 2^20 = 1MB */
  . = 0x100000;

  .data : {
    build/kernel.o (.data*)
    build/loader.o (.data*)
  }

  . = ALIGN(16);
  .bss : {

  }

  /* 2 * 2^20 = 2MB */
  . = 0x200000;
  PROVIDE(KERNEL_STACK_START = .);
  /* 8 * 2^20 = 8MB */
  . = 0x800000;
  PROVIDE(KERNEL_STACK_END = .);

  /* (3*16+12) * 2^20 = 60MB */
  /* . = 0x3c00000; */

  /* (4*16) * 2^20 64MB */
  . = 0x4000000;
  PROVIDE(user_entry_elf_start = .);
  .user_entry_elf : {
    pie-rust/build/user_entry_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(msg_demo_elf_start = .);
  .msg_demo_elf : {
    pie-rust/build/msg_demo_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(msg_demo_child_elf_start = .);
  .msg_demo_child_elf : {
    pie-rust/build/msg_demo_child_elf.o (.*)
  }

  PROVIDE(LINKER_CODE_END = .);
}
