#include "sys_const.h"
#include "mmu.h"

// https://www.rpi4os.com/part1-bootstrapping/

// ***************************************
// SCTLR_EL1, System Control Register (EL1)
// Architecture Reference Manual Section D13.2.118
// ***************************************
#define SCTLR_RESERVED (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)
#define USER_MASK_ACCESS (1 << 9)
#define SCTLR_WFE_WFI_ENABLED (1 << 18 | 1<<16)
#define SCTLR_IC_ENABLED (1 << 12)
#define SCTLR_DC_ENABLED (1 << 2)
#define SCTLR_MMU_ENABLED (1 << 0)
// #define SCTLR_VALUE_MMU_DISABLED (SCTLR_RESERVED | USER_MASK_ACCESS | SCTLR_WFE_WFI_ENABLED)
#define SCTLR_VALUE_MMU_DISABLED (SCTLR_RESERVED | USER_MASK_ACCESS | SCTLR_WFE_WFI_ENABLED | SCTLR_IC_ENABLED | SCTLR_DC_ENABLED)

// ***************************************
// HCR_EL2, Hypervisor Configuration Register (EL2)
// Architecture Reference Manual Section D13.2.48
// ***************************************
#define HCR_RW (1 << 31)

// ***************************************
// SPSR_EL3, Saved Program Status Register (EL3)
// Architecture Reference Manual Section C5.2.20
// ***************************************
#define SPSR_MASK_ALL (7 << 6)
#define SPSR_EL1h (5 << 0)
#define SPSR_VALUE (SPSR_MASK_ALL | SPSR_EL1h)

// ***************************************
// CNTKCTL_EL1, Counter-timer Kernel Control register
// Architecture Reference Manual Section D13.11.15
// ***************************************
#define CNTKCTL_VALUE ((1 << 9) | (1 << 8) | (1 << 1) | (1 << 0))

/* architectural feature access control register */
#define CPACR_EL1_FPEN    (1 << 21) | (1 << 20) // don't trap SIMD/FP registers
#define CPACR_EL1_ZEN     (1 << 17) | (1 << 16)  // don't trap SVE instructions
#define CPACR_EL1_VAL     (CPACR_EL1_FPEN | CPACR_EL1_ZEN)

/* MMU control register */
#define TCR_TG1_4K (2 << 30)
#define TCR_T1SZ ((64 - 48) << 16)
#define TCR_TG0_4K (0 << 14)
#define TCR_T0SZ (64 - 48)
#define TCR_EL1_VAL (TCR_TG1_4K | TCR_T1SZ | TCR_TG0_4K | TCR_T0SZ)

// ensure the linker puts this at the start of the kernel image
.section ".text.boot"
.global _start
_start:
    // check processor ID is zero (executing on main core), else loop
    mrs  x0, mpidr_el1
    and  x0, x0, #3
    cbnz x0, exit

    // are we already in EL1?
    mrs  x1, CurrentEL
    and  x1, x1, #8
    cbz  x1, el1_entry

    // otherwise, switch to EL1 by fake exception to return from
    ldr x2, =HCR_RW
    msr hcr_el2, x2

    ldr x3, =SPSR_VALUE
    msr spsr_el2, x3

    ldr x0, =CPACR_EL1_VAL
    msr cpacr_el1, x0

    /* MMU Config */
    ldr x0, =TCR_EL1_VAL
    msr tcr_el1, x0

    ldr x0, =MAIR_VALUE
    msr mair_el1, x0
    /* MMU Config */

    adr x4, el1_entry
    msr elr_el2, x4

    eret // -> el1_entry

el1_entry:
    // configure processor and mmu
    ldr x0, =SCTLR_VALUE_MMU_DISABLED
    msr sctlr_el1, x0

    adr x0, exception_vector_el1
    msr vbar_el1, x0

    // mask-out exceptions at EL1
    msr DAIFSet, #0b1111

    // initialize SP
    msr SPSel, #1
    ldr x0, =LINKER_CODE_START
    mov sp, x0

    // Perform bootstrapping
    // x0 will be the return value: kernel stack
    bl k_bootstrap_pgmgr

    // initialize SP
    msr SPSel, #1
    mov sp, x0

    // Jump to our main() routine in C (make sure it doesn't return)
    bl k_main

.global el1_reboot
el1_reboot:
    // use watchdog to reboot
    mov x2, 36
    mov x0, 28
    movk  x2, 0xfe10, lsl 16
    movk  x0, 0xfe10, lsl 16
    mov w3, 1
    mov w1, 32
    movk  w3, 0x5a00, lsl 16
    str w3, [x2]
    movk  w1, 0x5a00, lsl 16
    str w1, [x0]

exit:
    wfi
    b    exit

.global turn_on_mmu
turn_on_mmu:
  mrs x0, sctlr_el1
  mov x1, #SCTLR_MMU_ENABLED
  orr x0, x0, x1
  msr sctlr_el1, x0
  ret

.global memzero
memzero:
 str xzr, [x0], #8
 subs x1, x1, #8
 b.gt memzero
 ret

.global id_pgd_addr
id_pgd_addr:
  adrp x0, id_pgd
  ret
