#include "util.h"
#include "rpi.h"
#include "printf.h"

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

int util_memcmp(const void *ptr1, const void *ptr2, size_t num)
{
  const unsigned char *p1 = (const unsigned char *)ptr1;
  const unsigned char *p2 = (const unsigned char *)ptr2;
  for (size_t i = 0; i < num; i += 1)
  {
    if (p1[i] < p2[i])
      return -1;
    if (p1[i] > p2[i])
      return 1;
  }
  return 0;
}

void *util_memchr(const void *ptr, int value, size_t num)
{
  const unsigned char *p = (const unsigned char *)ptr;
  for (size_t i = 0; i < num; i += 1)
  {
    if (p[i] == (unsigned char)value)
    {
      return (void *)&p[i];
    }
  }
  return NULL;
}

char *util_strchr(const char *str, int c)
{
  for (size_t i = 0; str && str[i] != '\0'; i += 1)
  {
    if (str[i] == (char)c)
      return (char *)&str[i];
  }
  return NULL;
}

char *util_strrchr(const char *str, int c)
{
  char *x = NULL;
  for (size_t i = 0; str && str[i] != '\0'; i += 1)
  {
    if (str[i] == (char)c)
      x = (char *)&str[i];
  }
  return x;
}

static int8_t digits_map[] = {
    // - . / 0 1 2 3 4 5 6 7 8 9
    -1, -2, -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    // : ; < = > ? @
    -2, -2, -2, -2, -2, -2, -2,
    // A B C D E F
    10, 11, 12, 13, 14, 15,
    // G H I J K L M N O P Q R S T U V W X Y Z
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    // [ \ ] ^ _ `
    -2, -2, -2, -2, -2, -2,
    // a b c d e f
    10, 11, 12, 13, 14, 15,
    // g h i j k l m n o p q r s t u v w x y z
    // -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
};

static int8_t calc_digit(char c)
{
  if (c < '-' || c > 'f')
    return -2;
  return digits_map[c - '-'];
}

unsigned long int util_strtoul(const char *str, char **endptr, int base)
{
  unsigned long int ans = 0;
  while (1)
  {
    char c = *str;
    if (c == '\0')
      break;

    int8_t digit = calc_digit(c);
    if (digit < 0)
      break;

    if ((ULONG_MAX - (unsigned long int)digit) / base < ans)
    {
      *endptr = NULL;
      return __LONG_MAX__;
    }

    ans = ans * base + digit;
  }

  *endptr = (char *)str;
  return ans;
}

// void util_uint8_to_binary(uint8_t val, char *str) {
//   for (int i = 0; i < 8; i += 1) {
//     str[7 - i] = ((val & (1 << i)) >> i) + '0';
//   }
// }

// int util_decimal_to_unsigned(const char *str, size_t *idx)
// {
//   while (1)
//   {
//     if (str[*idx] == '\0')
//       return -1;
//     if (str[*idx] >= '0' && str[*idx] <= '9')
//       break;
//     *idx += 1;
//   }
//   int val = 0;
//   while (1)
//   {
//     if (str[*idx] < '0' || str[*idx] > '9')
//       return val;
//     val *= 10;
//     val += str[*idx] - '0';
//     *idx += 1;
//   }
// }

// int util_decimal_to_signed(const char *str, size_t *idx)
// {
//   int res;
//   while (1)
//   {
//     if (str[*idx] == '\0')
//       return INT32_MIN;
//     if (str[*idx] == '-')
//     {
//       *idx += 1;
//       res = util_decimal_to_unsigned(str, idx);
//       if (res == -1)
//         return INT32_MIN;
//       return -res;
//     }
//     res = util_decimal_to_unsigned(str, idx);
//     if (res == -1)
//       return INT32_MIN;
//     return res;
//   }
// }

// const void *util_memchr(const void *ptr, int value, size_t num)
// {
//   return util_memchr((void *)ptr, value, num);
// }
// const char *util_strchr(const char *str, int c)
// {
//   return util_strchr((char *)str, c);
// }
// const char *util_strrchr(const char *str, int c)
// {
//   return util_strrchr((char *)str, c);
// }