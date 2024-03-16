#include "loader_args.h"
#include "lib/include/util.h"

extern char user_entry_elf_start[];

static const char * resolve_elf_start(const char * program_name) {
  if (util_strcmp(program_name, "user_entry")) {
    return user_entry_elf_start;
  }
  return NULL;
}

struct LoaderArgs resolve_args(const char * args, uint64_t args_len)
{
  struct LoaderArgs loader_args = {
    .elf_start = NULL
  };

  uint64_t idx = 0;
  while (idx < args_len) {
    const char * key = &args[idx];
    uint64_t key_len = 0;
    while (key[key_len++] != '\0') {}
    const char * val = &args[idx + key_len];
    uint64_t val_len = 0;
    while (val[val_len++] != '\0') {}

    // Check key=PROGRAM
    if (util_strcmp(key, "PROGRAM")) {
      loader_args.elf_start = resolve_elf_start(val);
    }

    idx += (key_len + val_len);
  }

  return loader_args;
}