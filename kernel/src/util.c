#include "util.h"
#include "lib/include/rpi.h"
#include "lib/include/printf.h"

void print_debug(uint64_t reason)
{
  printf("Register print\r\nreason=%x\r\n", reason);
}

void print_debug_invalid_syscall(uint64_t reason)
{
  printf("Invalid Syscall Print\r\nreason=%x\r\n", reason);
}

const char *print_reg(uint64_t v)
{
  static char reg_str[] = "0x0000000000000000";
  for (int i = 0; i < 16; i += 1)
  {
    uint8_t digit = v % 16;
    if (digit >= 10)
    {
      reg_str[17 - i] = 'A' + (digit - 10);
    }
    else
    {
      reg_str[17 - i] = '0' + digit;
    }
    v /= 16;
  }
  return reg_str;
}
