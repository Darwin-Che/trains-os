#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MSB_POS(x)                   \
  __extension__({                    \
    uint8_t pos = sizeof(x) * 8 - 1; \
    for (; pos > 0; pos -= 1)        \
    {                                \
      if (x & ((typeof(x))1 << pos)) \
        break;                       \
    }                                \
    pos;                             \
  })

#define ALIGN_UP(val, sft) \
  (((val) + (1 << (sft)) - 1) & ~((typeof((val)))(1 << (sft)) - 1))

#define ALIGN_DOWN(val, sft) ((val) & ~((1 << sft) - 1))

#define NEXT_POW2(val64)     \
  __extension__({            \
    typeof(val64) v = val64; \
    v--;                     \
    v |= v >> 1;             \
    v |= v >> 2;             \
    v |= v >> 4;             \
    v |= v >> 8;             \
    v |= v >> 16;            \
    v |= v >> 32;            \
    v++;                     \
    v += (v == 0);           \
    v;                       \
  })
