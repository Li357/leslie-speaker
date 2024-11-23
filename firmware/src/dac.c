#include "dac.h"
#include "rcc.h"

void dac_init(uint32_t dac) {
  RCC->APB1ENR |= RCC_APB1ENR_DACEN;
  DAC->CR |= dac;
  if (dac & DAC_CH1) {
    DAC->CR |= DAC_SWTRIG << DAC_CH1_TSEL;
    DAC->CR |= DAC_CH1_EN | DAC_CH1_TEN;
  }
  if (dac & DAC_CH2) {
    DAC->CR |= DAC_SWTRIG << DAC_CH2_TSEL;
    DAC->CR |= DAC_CH2_EN | DAC_CH2_TEN;
  }
}

void dac_set(uint32_t ch1, uint32_t ch2) {
  uint32_t val = (ch2 << 16) | ch1;
  DAC->DHR12RD = val;
  DAC->SWTRIGR = DAC_CH1_SWTRIG | DAC_CH2_SWTRIG;
}
