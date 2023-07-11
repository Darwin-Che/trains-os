#ifndef ASSERT_H
#define ASSERT_H

#include <stdbool.h>
#include "printf.h"

void assert_fail();

#define ASSERT_MSG(COND, fmt, ...)                         \
  do                                                       \
  {                                                        \
    if (!(COND))                                           \
    {                                                      \
      printf(" |>_<| ASSERT_MSG %s:%d:%s(): " fmt,         \
             __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
      assert_fail();                                       \
    }                                                      \
  } while (0)

// void assert(bool b);
static inline void assert(bool b)
{
  if (!b)
  {
    // char msg[] = "Assert Failed!\n";
    // uart_puts(0, 0, msg, sizeof(msg) - 1);
    assert_fail();
  }
}

#endif