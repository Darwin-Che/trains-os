local0 .req x9
local1 .req x10

scratch0 .req x19
scratch1 .req x20

linkreg .req x30

.macro get_td regid
ldr \regid, =0x7FF00000
ldr \regid, [\regid, #0x00]
.endm

// #instructions = 10 or 15
.macro store_regs el // el0, el1, intr
.if \el == el1
  msr SPSel, #1
.else
  msr SPSel, #0
.endif

.if \el == intr
stp x1, x2, [sp, #-16]!
stp x3, x4, [sp, #-16]!
stp x5, x6, [sp, #-16]!
stp x7, x8, [sp, #-16]!
stp x9, x10, [sp, #-16]!
stp x11, x12, [sp, #-16]!
stp x13, x14, [sp, #-16]!
stp x15, x16, [sp, #-16]!
stp x17, x18, [sp, #-16]!
.endif
stp x19, x20, [sp, #-16]!
stp x21, x22, [sp, #-16]!
stp x23, x24, [sp, #-16]!
stp x25, x26, [sp, #-16]!
stp x27, x28, [sp, #-16]!
stp x29, x30, [sp, #-16]!
.endm

/*
  #instructions = 7 or 8
  Change scratch0, scratch1
  scratch0 = td
 */
.macro store_td excp // intr, sysc
get_td scratch0

// store pc
.if \excp == intr
mrs scratch1, elr_el1
str scratch1, [scratch0, #0x00]
.else
str linkreg, [scratch0, #0x00]
.endif

// store sp
mov scratch1, sp
str scratch1, [scratch0, #0x08]

// store pstate if needed
mrs scratch1, spsr_el1
str scratch1, [scratch0, #0x10]
.endm

/*
  #instructions = 8
  Change scratch0, scratch1
  scratch0 = td
 */
.macro extract_td
get_td scratch0

// extract pc
ldr scratch1, [scratch0, #0x00]
msr elr_el1, scratch1

// extract sp
ldr scratch1, [scratch0, #0x08]
msr	sp_el0, scratch1

// extract pstate
ldr scratch1, [scratch0, #0x10]
msr spsr_el1, scratch1
.endm

/*
  #instructions = 7 or 19
  If el = el0, scratch0 should be td address
  At exit, x0 is the td address
 */
.macro extract_regs el // el0, el1
.if \el == el0
msr SPSel, #0
mov x0, scratch0
.else
msr SPSel, #1
.endif

ldp x29, x30, [sp], #16
ldp x27, x28, [sp], #16
ldp x25, x26, [sp], #16
ldp x23, x24, [sp], #16
ldp x21, x22, [sp], #16
ldp x19, x20, [sp], #16
.if \el == el0
// check how to restore the registers, -> int or svc (from td)
ldr local1, [x0, #0x18]
cbnz local1, extract_regs_el0_sysc
ldp x17, x18, [sp], #16
ldp x15, x16, [sp], #16
ldp x13, x14, [sp], #16
ldp x11, x12, [sp], #16
ldp x9, x10, [sp], #16
ldp x7, x8, [sp], #16
ldp x5, x6, [sp], #16
ldp x3, x4, [sp], #16
ldp x1, x2, [sp], #16

extract_regs_el0_sysc:
.endif
.endm
