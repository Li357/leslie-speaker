#pragma once

#include "stm32f7.h"

#define DMA_SxCR_PL_SHIFT    (16)
#define DMA_SxCR_MSIZE_SHIFT (13)
#define DMA_SxCR_PSIZE_SHIFT (11)
#define DMA_SxCR_CHSEL_SHIFT (25)
#define DMA_SxCR_DIR_SHIFT   (6)
#define DMA_SxCR_MINC        (1UL << 10)
#define DMA_SxCR_CIRC        (1UL << 8)
#define DMA_SxCR_EN          (1UL << 0)

typedef volatile struct {
  uint32_t CR;
  uint32_t NDTR;
  uint32_t PAR;
  uint32_t M0AR;
  uint32_t M1AR;
  uint32_t FCR;
} dma_stream_reg_t;

typedef volatile struct {
  uint32_t LISR;
  uint32_t HISR;
  uint32_t LIFCR;
  uint32_t HIFCR;
  dma_stream_reg_t streams[8];
} dma_reg_t;

#define DMA1 ((dma_reg_t *)DMA1_BASE)
#define DMA2 ((dma_reg_t *)DMA2_BASE)
