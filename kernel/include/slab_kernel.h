#ifndef SLAB_KERNEL_H
#define SLAB_KERNEL_H

#include <stdint.h>

void *slab_kernel_func_alloc_page(uint8_t p, uint16_t flags);

#endif