#ifndef C_RING_BUFFER_H
#define C_RING_BUFFER_H

#include "util.h"
#include "assert.h"
#define RB_TYPE(ELEM_T, CAP) \
  struct                     \
  {                          \
    size_t head;             \
    size_t size;             \
    size_t capacity;         \
    ELEM_T array[CAP];       \
  }

#define RB_INIT(RB, CAP)                                \
  do                                                    \
  {                                                     \
    (RB)->capacity = CAP;                               \
    (RB)->head = CAP - 1;                               \
    (RB)->size = 0;                                     \
    util_memset((RB)->array, 0x0, sizeof((RB)->array)); \
  } while (0)

// ELEM should be a pointer type,
// It will be set to point the the pushed element,
// Then initialize the element through that pointer
#define RB_PUSH_PRE(RB, ELEM_PTR)                                        \
  do                                                                     \
  {                                                                      \
    if ((RB)->size >= (RB)->capacity)                                    \
    {                                                                    \
      ELEM_PTR = NULL;                                                   \
      break;                                                             \
    }                                                                    \
    (RB)->head += 1;                                                     \
    (RB)->head %= (RB)->capacity;                                        \
    ASSERT_MSG((RB)->size < (RB)->capacity, "RB size exceeds capacity"); \
    (RB)->size += 1;                                                     \
    ELEM_PTR = &(RB)->array[(RB)->head];                                 \
  } while (0)

#define RB_PEEK(RB, ELEM_PTR)                                                       \
  do                                                                                \
  {                                                                                 \
    if ((RB)->size == 0)                                                            \
    {                                                                               \
      ELEM_PTR = NULL;                                                              \
    }                                                                               \
    else                                                                            \
    {                                                                               \
      size_t idx = ((RB)->head + (RB)->capacity + 1 - (RB)->size) % (RB)->capacity; \
      ELEM_PTR = &(RB)->array[idx];                                                 \
    }                                                                               \
  } while (0)

#define RB_POP(RB)   \
  do                 \
  {                  \
    (RB)->size -= 1; \
  } while (0)

#define RB_LEN(RB) \
  (RB)->size

#endif