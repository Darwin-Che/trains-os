#include "stack.h"

/*
  tid is in the range [0, USER_STACK_ARRAY_CNT)
  returns stackend
*/
void *k_stack_get_stackend(int64_t stack_id)
{
  return (void *)(USER_STACK_ARRAY_START + STACK_SZ * (stack_id + 1));
}