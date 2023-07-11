#include "util.h"
#include "rpi.h"
#include "printf.h"

// void util_uint8_to_binary(uint8_t val, char *str) {
//   for (int i = 0; i < 8; i += 1) {
//     str[7 - i] = ((val & (1 << i)) >> i) + '0';
//   }
// }

int util_decimal_to_unsigned(const char *str, size_t *idx)
{
  while (1)
  {
    if (str[*idx] == '\0')
      return -1;
    if (str[*idx] >= '0' && str[*idx] <= '9')
      break;
    *idx += 1;
  }
  int val = 0;
  while (1)
  {
    if (str[*idx] < '0' || str[*idx] > '9')
      return val;
    val *= 10;
    val += str[*idx] - '0';
    *idx += 1;
  }
}

int util_decimal_to_signed(const char *str, size_t *idx)
{
  int res;
  while (1)
  {
    if (str[*idx] == '\0')
      return INT32_MIN;
    if (str[*idx] == '-')
    {
      *idx += 1;
      res = util_decimal_to_unsigned(str, idx);
      if (res == -1)
        return INT32_MIN;
      return -res;
    }
    res = util_decimal_to_unsigned(str, idx);
    if (res == -1)
      return INT32_MIN;
    return res;
  }
}

bool util_strcmp(const char *s1, const char *s2)
{
  if (s1 == NULL && s2 == NULL)
    return true;
  if (s1 == NULL || s2 == NULL)
    return false;

  size_t i = 0;
  while (1)
  {
    if (s1[i] != s2[i])
    {
      return false;
    }
    if (s1[i] == '\0')
    {
      return true;
    }

    i += 1;
  }
  return true;
}

bool util_strbegin(const char *s, const char *p)
{
  if (s == NULL || p == NULL)
    return false;

  size_t i = 0;
  while (1)
  {
    if (p[i] == '\0')
    {
      return true;
    }
    if (s[i] != p[i])
    {
      return false;
    }

    i += 1;
  }
  return true;
}

void util_strcpy(char *s1, const char *s2)
{
  util_memcpy(s1, s2, util_strlen(s2) + 1);
}

void util_strncpy(char *s1, size_t n, const char *s2)
{
  size_t l2 = util_strlen(s2);
  if (l2 > n - 1)
  {
    l2 = n - 1;
  }
  util_memcpy(s1, s2, l2);
  s1[l2] = '\0';
}
