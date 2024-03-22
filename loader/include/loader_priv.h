#ifndef L_LOADER_PRIV_H
#define L_LOADER_PRIV_H

#include "lib/include/printf.h"
#include <stdint.h>

// #define DEBUG_LOADER

#if defined(DEBUG_LOADER)
#define DEBUG_LOADER_PRINTF(fmt, ...)                                               \
  do                                                                        \
  {                                                                         \
    printf(fmt, ##__VA_ARGS__);                                                  \
  } while (0)
#else
#define DEBUG_LOADER_PRINTF(fmt, ...) /* Don't do anything in release builds */
#endif


struct LoaderArgs
{
  const char * elf_start;
};

struct LoaderArgs resolve_args(const char * args, uint64_t args_len);

#endif