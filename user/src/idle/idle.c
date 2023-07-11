#include <stdint.h>
#include "common/timer.h"
#include "kernel/uapi.h"
#include "common/dashboard.h"
#include "common/printf.h"

void idle_task()
{
  while (1)
  {
    asm volatile("wfi");
  }
}