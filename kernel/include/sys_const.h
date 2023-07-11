#ifndef K_SYS_CONST_H
#define K_SYS_CONST_H

/*
0x0010_0000 is 1MB

0x8000_0000
-
0x4000_0000

can have 400 stacks

The Hard coded region 0x7FF0_0000 - 0x8000_0000

The kernel stack is 0x7E00_0000 - 0x7F00_0000

First user stack is 0x4000_0000 - 0x4010_0000
...
How many user stacks do we have? ((0x7E00_0000 - 0x4000_0000) / STACK_SZ) - 1
// Last user stack is 0x7DF0_0000 - 0x7E00_0000
*/

// Duplicate definition in context_macro.S
#define STACK_SZ 0x100000

#define CUR_ACTIVE_TD_PTR 0x7FF00000
#define KERNEL_STACKEND 0x7F000000

#define USER_STACK_ARRAY_START 0x40000000
#define USER_STACK_ARRAY_END 0x7E000000
#define USER_STACK_ARRAY_CNT \
  ((USER_STACK_ARRAY_END - USER_STACK_ARRAY_START) / STACK_SZ) - 1

#endif
