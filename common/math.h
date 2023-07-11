#ifndef C_MATH_H
#define C_MATH_H

#include <stdint.h>

static inline uint64_t math_sqrt(uint64_t a)
{
  uint64_t x = a;
  while (x * x > a)
  {
    x = (x + a / x) / 2;
  }
  return x;
}

#define MATH_ABS(x) ((x) < 0 ? -(x) : (x))

struct MathQuadrRoots
{
  int cnt;
  int64_t r1; // Usually the bigger one
  int64_t r2;
};

struct MathQuadrRoots math_solve_quadr(int64_t a, int64_t b, int64_t c);

static inline uint32_t math_rand_u32()
{
  static uint32_t z1 = 12345, z2 = 12345, z3 = 12345, z4 = 12345;
  uint32_t b;
  b = ((z1 << 6) ^ z1) >> 13;
  z1 = ((z1 & 4294967294U) << 18) ^ b;
  b = ((z2 << 2) ^ z2) >> 27;
  z2 = ((z2 & 4294967288U) << 2) ^ b;
  b = ((z3 << 13) ^ z3) >> 21;
  z3 = ((z3 & 4294967280U) << 7) ^ b;
  b = ((z4 << 3) ^ z4) >> 12;
  z4 = ((z4 & 4294967168U) << 13) ^ b;
  return (z1 ^ z2 ^ z3 ^ z4);
}

#endif