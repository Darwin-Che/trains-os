#ifndef K_HASHTABLE_H
#define K_HASHTABLE_H

#include "lib/include/hashtable.h"

#define K_TASK_HT_BUCKSZ 512
#define K_TASK_HT_CAPACITY 1024

struct kTaskDsp;

HT_ELEM_TYPE(int64_t, struct kTaskDsp *, kHtTaskElem);

HT_TYPE(kHtTaskElem, K_TASK_HT_BUCKSZ, K_TASK_HT_CAPACITY, kHtTask);

// uint64_t hash_func_task(int64_t *tid);

// bool cmp_func_task(int64_t *k1, int64_t *k2);

#define HT_HASH_FUNC(ELEM) _Generic((ELEM), struct kHtTaskElem : hash_func_task)

#define HT_CMP_FUNC(ELEM) _Generic((ELEM), struct kHtTaskElem : cmp_func_task)

static inline uint64_t hash_func_task(int64_t *tid) {
  return *tid;
  uint64_t _ho_i;
  const unsigned char *_ho_key = (const unsigned char *)(tid);
  uint64_t hashv = 0;
  for (_ho_i = 0; _ho_i < 8; _ho_i++) {
    hashv += _ho_key[_ho_i];
    hashv += (hashv << 10);
    hashv ^= (hashv >> 6);
  }
  hashv += (hashv << 3);
  hashv ^= (hashv >> 11);
  hashv += (hashv << 15);
  return hashv;
}

static inline bool cmp_func_task(int64_t *k1, int64_t *k2) {
  return *k1 == *k2;
}

#endif