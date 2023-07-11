#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

static inline size_t util_strlen(const char *str)
{
  size_t idx = 0;
  while (str && str[idx])
  {
    idx += 1;
  }
  return idx;
}

// return the length parsed
static inline size_t util_int_to_decimal(uint32_t val, char *str)
{
  size_t len = 0;
  uint32_t val_tmp = val;
  do
  {
    val_tmp /= 10;
    len += 1;
  } while (val_tmp);

  for (int i = len - 1; i >= 0; i -= 1)
  {
    str[i] = val % 10 + '0';
    val /= 10;
  }

  str[len] = '\0';

  return len;
}

int util_decimal_to_unsigned(const char *str, size_t *idx);
int util_decimal_to_signed(const char *str, size_t *idx);
bool util_strcmp(const char *s1, const char *s2); // true if the same
bool util_strbegin(const char *s, const char *p);
void util_strcpy(char *s1, const char *s2);
void util_strncpy(char *s1, size_t n, const char *s2);

// define our own memset to avoid SIMD instructions emitted from the compiler
static inline void *util_memset(void *s, int c, size_t n)
{
  for (char *it = (char *)s; n > 0; --n)
    *it++ = c;
  return s;
}

// define our own memcpy to avoid SIMD instructions emitted from the compiler
static inline void *util_memcpy(void *restrict dest, const void *restrict src,
                                size_t n)
{
  char *sit = (char *)src;
  char *cdest = (char *)dest;
  for (size_t i = 0; i < n; ++i)
    *(cdest++) = *(sit++);
  return dest;
}

#endif