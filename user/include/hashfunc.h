#ifndef U_HASHFUNC_H
#define U_HASHFUNC_H

#include <stdint.h>

static uint64_t hashfunc_string(const char *str) {
  uint64_t _ho_i = 0;
  const unsigned char *_ho_key = (const unsigned char *)(str);
  uint64_t hashv = 0;
  while (_ho_key[_ho_i] != '\0') {
    hashv += _ho_key[_ho_i];
    hashv += (hashv << 10);
    hashv ^= (hashv >> 6);
    _ho_i += 1;
  }
  hashv += (hashv << 3);
  hashv ^= (hashv >> 11);
  hashv += (hashv << 15);
  return hashv;
}

#endif