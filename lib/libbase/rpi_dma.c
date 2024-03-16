#include "rpi.h"

struct RPI_DMA_CONFIG
{
  uint32_t chnl_id;
};

struct RPI_DMA * dma_setup_chnl(const struct RPI_DMA_CONFIG * config)
{
}

void dma_ctl_append(uint32_t id, uint32_t ti, void * src_addr, void * dest_addr, uint32_t tf_len)
{
}

void dma_ctl_start(uint32_t id)
{
}

void dma_ctl_stop(uint32_t id) {

}
