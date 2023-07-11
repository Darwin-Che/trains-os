#ifndef HEAP_H
#define HEAP_H
#include <stdint.h>
#include <stdbool.h>

#define HEAP_TYPE(ELEM_T, CAPACITY, HP_T) \
  struct HP_T                             \
  {                                       \
    struct ELEM_T *heap_array[CAPACITY];  \
    int64_t count;                        \
    int64_t capacity;                     \
    uint64_t global_insertion_count;      \
  };

#define HEAP_INIT(HEAP_PTR, CAPACITY)       \
  do                                        \
  {                                         \
    (HEAP_PTR)->count = 0;                  \
    (HEAP_PTR)->capacity = CAPACITY;        \
    (HEAP_PTR)->global_insertion_count = 0; \
  } while (0)

// can do memset to 0

#define HEAPIFY_BOTTOM_TOP(HEAP_T, ELEM_T, KEY_FIELD_NAME, CMP_FUNC)                                    \
  static void heapify_bottom_top##HEAP_T(struct HEAP_T *h, int64_t index)                               \
  {                                                                                                     \
    struct ELEM_T *temp;                                                                                \
    int64_t parent_node;                                                                                \
    if (index >= 1)                                                                                     \
    {                                                                                                   \
      parent_node = (index - 1) / 2;                                                                    \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
      parent_node = 0;                                                                                  \
    }                                                                                                   \
    if (CMP_FUNC((h)->heap_array[index]->KEY_FIELD_NAME, (h)->heap_array[parent_node]->KEY_FIELD_NAME)) \
    {                                                                                                   \
      temp = (h)->heap_array[parent_node];                                                              \
      (h)->heap_array[parent_node] = (h)->heap_array[index];                                            \
      (h)->heap_array[index] = temp;                                                                    \
      heapify_bottom_top##HEAP_T(h, parent_node);                                                       \
    }                                                                                                   \
  }

#define HEAPIFY_TOP_BOTTOM(HEAP_T, ELEM_T, KEY_FIELD_NAME, CMP_FUNC)                                             \
  static void heapify_top_bottom##HEAP_T(struct HEAP_T *h, int64_t parent_node)                                  \
  {                                                                                                              \
    struct ELEM_T *temp;                                                                                         \
    int64_t left = parent_node * 2 + 1;                                                                          \
    int64_t right = parent_node * 2 + 2;                                                                         \
    int64_t min;                                                                                                 \
    if (left >= h->count || left < 0)                                                                            \
    {                                                                                                            \
      left = -1;                                                                                                 \
    }                                                                                                            \
    if (right >= h->count || right < 0)                                                                          \
    {                                                                                                            \
      right = -1;                                                                                                \
    }                                                                                                            \
    if (left != -1 && CMP_FUNC(h->heap_array[left]->KEY_FIELD_NAME, h->heap_array[parent_node]->KEY_FIELD_NAME)) \
    {                                                                                                            \
      min = left;                                                                                                \
    }                                                                                                            \
    else                                                                                                         \
    {                                                                                                            \
      min = parent_node;                                                                                         \
    }                                                                                                            \
                                                                                                                 \
    if (right != -1 && CMP_FUNC(h->heap_array[right]->KEY_FIELD_NAME, h->heap_array[min]->KEY_FIELD_NAME))       \
    {                                                                                                            \
      min = right;                                                                                               \
    }                                                                                                            \
                                                                                                                 \
    if (min != parent_node)                                                                                      \
    {                                                                                                            \
      temp = h->heap_array[min];                                                                                 \
      h->heap_array[min] = h->heap_array[parent_node];                                                           \
      h->heap_array[parent_node] = temp;                                                                         \
      heapify_top_bottom##HEAP_T(h, min);                                                                        \
    }                                                                                                            \
  }

#define HEAP_INSERT(HEAP_T, HEAP_PTR, KEY_FIELD_NAME, ELEM_PTR)                   \
  do                                                                              \
  {                                                                               \
    if ((HEAP_PTR)->count < (HEAP_PTR)->capacity)                                 \
    {                                                                             \
      (ELEM_PTR)->KEY_FIELD_NAME.insert_val = (HEAP_PTR)->global_insertion_count; \
      (HEAP_PTR)->heap_array[(HEAP_PTR)->count] = ELEM_PTR;                       \
      heapify_bottom_top##HEAP_T(HEAP_PTR, (HEAP_PTR)->count);                    \
      (HEAP_PTR)->count += 1;                                                     \
      (HEAP_PTR)->global_insertion_count += 1;                                    \
    }                                                                             \
  } while (0)

#define HEAP_INSERT_U(HEAP_T, HEAP_PTR, KEY_FIELD_NAME, ELEM_PTR)          \
  do                                                                              \
  {                                                                               \
    if ((HEAP_PTR)->count < (HEAP_PTR)->capacity)                                 \
    {                                                                             \
      (HEAP_PTR)->heap_array[(HEAP_PTR)->count] = ELEM_PTR;                       \
      heapify_bottom_top##HEAP_T(HEAP_PTR, (HEAP_PTR)->count);                    \
      (HEAP_PTR)->count += 1;                                                     \
    }                                                                             \
  } while (0)

#define HEAP_POP(HEAP_T, HEAP_PTR, ELEM_PTR)                                   \
  do                                                                           \
  {                                                                            \
    if ((HEAP_PTR)->count == 0)                                                \
    {                                                                          \
      ELEM_PTR = NULL;                                                         \
      break;                                                                   \
    }                                                                          \
    ELEM_PTR = (HEAP_PTR)->heap_array[0];                                      \
    (HEAP_PTR)->heap_array[0] = (HEAP_PTR)->heap_array[(HEAP_PTR)->count - 1]; \
    (HEAP_PTR)->count -= 1;                                                    \
    heapify_top_bottom##HEAP_T(HEAP_PTR, 0);                                   \
  } while (0);

#define HEAP_PEEK(HEAP_PTR, ELEM_PTR)       \
  do                                        \
  {                                         \
    if ((HEAP_PTR)->count == 0)             \
    {                                       \
      ELEM_PTR = NULL;                      \
    }                                       \
    else                                    \
    {                                       \
      ELEM_PTR = (HEAP_PTR)->heap_array[0]; \
    }                                       \
  } while (0);
#endif