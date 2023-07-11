#include "util.h"
#include "common/rpi.h"

void print_debug(uint64_t reason)
{
  char msg1[] = "Register print\r\n";
  uart_puts(0, 0, msg1, sizeof(msg1) - 1);
  uart_puts(0, 0, print_reg(reason), 18);
  uart_puts(0, 0, "\r\n", 2);
  uart_getc(0, 0);
}

void print_debug_invalid_syscall(uint64_t reason)
{
  char msg1[] = "Invalid Syscall Print\r\n";
  uart_puts(0, 0, msg1, sizeof(msg1) - 1);
  uart_puts(0, 0, print_reg(reason), 18);
  uart_puts(0, 0, "\r\n", 2);
  uart_getc(0, 0);
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
