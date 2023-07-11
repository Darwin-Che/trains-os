#ifndef C_HASHTABLE_H
#define C_HASHTABLE_H

#include "assert.h"
#include "printf.h"
#include "util.h"
#include "utlist.h"

#define HT_ELEM_TYPE(KEY_T, VALUE_T, ELEM_T)                                   \
  struct ELEM_T {                                                              \
    KEY_T key;                                                                 \
    VALUE_T val;                                                               \
    struct ELEM_T *ht_next;                                                    \
  }

#define HT_TYPE(ELEM_T, BUCKSZ, CAPACITY, HT_T)                                \
  struct HT_T {                                                                \
    size_t bucksz;                                                             \
    size_t capacity;                                                           \
    struct ELEM_T *buckets[BUCKSZ];                                            \
    struct ELEM_T objects[CAPACITY];                                           \
    struct ELEM_T *free_list;                                                  \
  }

#define HT_INIT(HT, BUCKSZ, CAPACITY)                                          \
  do {                                                                         \
    (HT)->bucksz = BUCKSZ;                                                     \
    (HT)->capacity = CAPACITY;                                                 \
    util_memset((HT)->buckets, 0x0, sizeof((HT)->buckets));                    \
    util_memset((HT)->objects, 0x0, sizeof((HT)->objects));                    \
    (HT)->free_list = NULL;                                                    \
    for (int i = (HT)->capacity; i >= 0; i -= 1)                               \
      LL_PREPEND2((HT)->free_list, &(HT)->objects[i], ht_next);                \
  } while (0)

#define HT_IDX(HT, KEY_PTR)                                                    \
  ((HT_HASH_FUNC((HT)->objects[0]))(KEY_PTR) % ((HT)->bucksz))

#define HT_CMP(HT, KEY_PTR1, KEY_PTR2)                                         \
  ((HT_CMP_FUNC((HT)->objects[0]))(KEY_PTR1, KEY_PTR2))

#define HT_GET_FREE(HT, ELEM_PTR)                                              \
  do {                                                                         \
    ELEM_PTR = (HT)->free_list;                                                \
    LL_DELETE2((HT)->free_list, ELEM_PTR, ht_next);                            \
  } while (0)

#define HT_PUT_FREE(HT, ELEM_PTR)                                              \
  do {                                                                         \
    LL_PREPEND2((HT)->free_list, ELEM_PTR, ht_next);                           \
  } while (0)

#define HT_INSERT(HT, ELEM_PTR)                                                \
  do {                                                                         \
    uint64_t hv = HT_IDX(HT, &(ELEM_PTR)->key);                                \
    LL_PREPEND2((HT)->buckets[hv], ELEM_PTR, ht_next);                         \
  } while (0)

#define HT_INSERT_KV(HT, K, V)                                                 \
  do {                                                                         \
    typeof((HT)->free_list) elem_ptr;                                          \
    HT_GET_FREE(HT, elem_ptr);                                                 \
    if (elem_ptr == NULL) {                                                    \
      printf("HT_INSERT_KV cannot allocate!");                                 \
      assert_fail();                                                           \
    }                                                                          \
    elem_ptr->key = K;                                                         \
    elem_ptr->val = V;                                                         \
    HT_INSERT(HT, elem_ptr);                                                   \
  } while (0)

#define HT_FIND(HT, KEY_PTR, ELEM_PTR)                                         \
  do {                                                                         \
    uint64_t hv = HT_IDX(HT, KEY_PTR);                                         \
    LL_FOREACH2((HT)->buckets[hv], ELEM_PTR, ht_next) {                        \
      if (HT_CMP(HT, &(ELEM_PTR)->key, KEY_PTR))                               \
        break;                                                                 \
    }                                                                          \
  } while (0)

#define HT_DEL(HT, ELEM_PTR)                                                   \
  do {                                                                         \
    uint64_t hv = HT_IDX(HT, &(ELEM_PTR)->key);                                \
    LL_DELETE2((HT)->buckets[hv], ELEM_PTR, ht_next);                          \
    HT_PUT_FREE(HT, ELEM_TR);                                                  \
  } while (0)

#define HT_DEL_BY_KEY(HT, KEY_PTR)                                             \
  do {                                                                         \
    uint64_t hv = HT_IDX(HT, KEY_PTR);                                         \
    typeof((HT)->free_list) elem_ptr;                                          \
    LL_FOREACH2((HT)->buckets[hv], elem_ptr, ht_next) {                        \
      if (HT_CMP(HT, &(elem_ptr)->key, KEY_PTR)) {                             \
        LL_DELETE2((HT)->buckets[hv], elem_ptr, ht_next);                      \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    HT_PUT_FREE(HT, elem_ptr);                                                 \
  } while (0)

#endif