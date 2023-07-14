#include <stdint.h>
#include "lib/include/timer.h"
#include "kernel/uapi.h"
#include "lib/include/dashboard.h"
#include "lib/include/printf.h"

void idle_task()
{
  while (1)
  {
    asm volatile("wfi");
  }
}