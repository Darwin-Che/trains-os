#ifndef U_KE_PRINT_H
#define U_KE_PRINT_H

#include "kernel/uapi.h"
#include "lib/include/assert.h"
#include "lib/include/rpi.h"

#define STOP_ALL_MARKLIN 97

#define U_ASSERT_MSG(COND, fmt, ...)                                           \
  do {                                                                         \
    char debug_msg[2048];                                                      \
    if (!(COND)) {                                                             \
      sprintf(debug_msg, " |>_<| U_ASSERT_MSG %s:%d:%s(): " fmt, __FILE__,     \
              __LINE__, __func__, ##__VA_ARGS__);                              \
      printf(debug_msg);                                                       \
      assert_fail();                                                           \
    }                                                                          \
  } while (0)

#define DEBUG_DELAY 5

#ifndef DEBUG_DELAY

#define U_PRINT(fmt, ...)                                                      \
  do {                                                                         \
    char debug_msg[2048];                                                      \
    sprintf(debug_msg, "U_PRINT %s:%d:%s(): " fmt, __FILE__, __LINE__,         \
            __func__, ##__VA_ARGS__);                                          \
    printf(debug_msg);                                                         \
  } while (0)

#else

#define U_PRINT(fmt, ...)                                                      \
  do {                                                                         \
    char debug_msg[2048];                                                      \
    sprintf(debug_msg, "U_PRINT_DELAY %s:%d:%s(): " fmt, __FILE__, __LINE__,   \
            __func__, ##__VA_ARGS__);                                          \
    printf(debug_msg);                                                         \
  } while (0)

#endif

#endif