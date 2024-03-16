#ifndef L_LOADER_ARGS_H
#define L_LOADER_ARGS_H

#include <stdint.h>

struct LoaderArgs
{
  const char * elf_start;
};

struct LoaderArgs resolve_args(const char * args, uint64_t args_len);

#endif