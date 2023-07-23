# Physical Memory Layout

## TODO

- How large a stack for the first user task?

## Boot

At boot time, we load the built image on to RAM, and starts execution at `0x80000`.

The page manager will be initialized in the bootstrap process.

The bootstrap process is written in C and needs a stack.
However, this stack is not managed by the page manager, we need to find a easy way to pick the space for the stack.
Because any function/algorithm picking this space must not use a stack, so I prefer to have it fixed by the linker.
Right now, all of this (physical memory segments) are hardcoded in `kernel/src/bootstrap.c`.
This is not desired and there might be bugs using it on a different address map.

The desired memory layout is
```
0x0 - 0x80_000 : free space
0x80_000 - 0x100_000 : code + data for kernel & lib
0x100_000 - 0xWHATEVER : code + data for user
0xWHATEVER - 0x80_000_000 : free space
```

I will start with 
The desired memory layout is
```
0x0 - 0x80_000 : free space
0x80_000 - 0xWHATEVER : code + data for all
0xWHATEVER - 0x80_000_000 : free space
```