#pragma once

#include "stm32f7.h"

#define DAC_CH1        (1)
#define DAC_CH2        (1UL << 16)

#define DAC_SWTRIG     (0b111)

#define DAC_CH1_TSEL   (3)
#define DAC_CH1_EN     (1UL << 0)
#define DAC_CH1_TEN    (1UL << 2)
#define DAC_CH2_TSEL   (19)
#define DAC_CH2_EN     (1UL << 16)
#define DAC_CH2_TEN    (1UL << 18)

#define DAC_CH1_SWTRIG (1UL << 0)
#define DAC_CH2_SWTRIG (1UL << 1)

typedef volatile struct {
  uint32_t CR;
  uint32_t SWTRIGR;
  uint32_t DHR12R1;
  uint32_t DHR12L1;
  uint32_t DHR8R1;
  uint32_t DHR12R2;
  uint32_t DHR12L2;
  uint32_t DHR8R2;
  uint32_t DHR12RD;
  uint32_t DHR12LD;
  uint32_t DHR8RD;
  uint32_t DOR1;
  uint32_t DOR2;
  uint32_t SR;
} dac_reg_t;

#define DAC ((dac_reg_t *)DAC_BASE)

void dac_init(uint32_t dac);
void dac_set(uint32_t ch1, uint32_t ch2);
