#include "loader_priv.h"
#include "lib/include/util.h"
#include "lib/include/printf.h"

extern char user_entry_elf_start[];
extern char msg_demo_elf_start[];
extern char msg_demo_child_elf_start[];
extern char name_server_elf_start[];
extern char name_server_demo_elf_start[];
extern char rpi_elf_start[];
extern char rpi_uart_elf_start[];
extern char rpi_uart_intr_broker_elf_start[];
extern char rpi_bluetooth_commander_elf_start[];
extern char rpi_bluetooth_hci_rx_elf_start[];
extern char rpi_bluetooth_gatt_elf_start[];
extern char clock_server_elf_start[];
extern char clock_server_helper_elf_start[];
extern char clock_notifier_elf_start[];
extern char encoder_server_elf_start[];

static const char * resolve_elf_start(const char * program_name) {
  if (util_strcmp(program_name, "user_entry")) {
    return user_entry_elf_start;
  }
  if (util_strcmp(program_name, "msg_demo")) {
    return msg_demo_elf_start;
  }
  if (util_strcmp(program_name, "msg_demo_child")) {
    return msg_demo_child_elf_start;
  }
  if (util_strcmp(program_name, "name_server")) {
    return name_server_elf_start;
  }
  if (util_strcmp(program_name, "name_server_demo")) {
    return name_server_demo_elf_start;
  }
  if (util_strcmp(program_name, "rpi")) {
    return rpi_elf_start;
  }
  if (util_strcmp(program_name, "rpi_uart")) {
    return rpi_uart_elf_start;
  }
  if (util_strcmp(program_name, "rpi_uart_intr_broker")) {
    return rpi_uart_intr_broker_elf_start;
  }
  if (util_strcmp(program_name, "rpi_bluetooth_commander")) {
    return rpi_bluetooth_commander_elf_start;
  }
  if (util_strcmp(program_name, "rpi_bluetooth_hci_rx")) {
    return rpi_bluetooth_hci_rx_elf_start;
  }
  if (util_strcmp(program_name, "rpi_bluetooth_gatt")) {
    return rpi_bluetooth_gatt_elf_start;
  }
  if (util_strcmp(program_name, "clock_server")) {
    return clock_server_elf_start;
  }
  if (util_strcmp(program_name, "clock_server_helper")) {
    return clock_server_helper_elf_start;
  }
  if (util_strcmp(program_name, "clock_notifier")) {
    return clock_notifier_elf_start;
  }
    if (util_strcmp(program_name, "encoder_server")) {
    return encoder_server_elf_start;
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
    printf("LOADER_ARGS KEY = %s\r\n", key);

    const char * val = &args[idx + key_len];
    uint64_t val_len = 0;
    while (val[val_len++] != '\0') {}
    printf("LOADER_ARGS VALUE = %s\r\n", val);

    // Check key=PROGRAM
    if (util_strcmp(key, "PROGRAM")) {
      loader_args.elf_start = resolve_elf_start(val);
    }

    idx += (key_len + val_len);
  }

  return loader_args;
}