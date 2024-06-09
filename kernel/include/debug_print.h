#ifndef K_DEBUG_PRINT_H
#define K_DEBUG_PRINT_H

#include "lib/include/printf.h"
#include "lib/include/rpi.h"

// #define DEBUG 1

#if defined(DEBUG) && DEBUG > 0
#define DEBUG_PRINT(fmt, ...)                                               \
  do                                                                        \
  {                                                                         \
    printf("------- DEBUG: %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, \
           ##__VA_ARGS__);                                                  \
  } while (0)
#else
#define DEBUG_PRINT(fmt, ...) /* Don't do anything in release builds */
#endif

#endif
