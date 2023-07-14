ENTRY(_start)           /* ELF entry symbol - no strictly needed */

SECTIONS {
  /* Page Tabl */
  /* Arbitrary, aligned with 4KB (0x1000) */
  . = 0x40000; /* 2^18 */
  id_pgd = .;
	.pd_pgd : { . += (6 * (1 << 12)); }

  /* Bootloader And Code Segment */
  . = 0x80000; /* 2^19 */
	.text : {
		KEEP("build/kernel.o"(.text.boot))
    build/kernel.o (.text*)
    build/user.o (.text*)
    build/lib.o (.text*)
	}

  /* Data Segment At 16MB */
  /* Separate by 2MB - Page Table Entry */
  . = 0x1000000; /* 2^24 */
  data_start = .;
  .data : {
    build/kernel.o (.*)
    build/user.o (.*)
    build/lib.o (.*)
  }
}
