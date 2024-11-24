#include "adc.h"
#include "nvic.h"
#include "rcc.h"

void adc_init(adc_reg_t *adc) {
  uint32_t en =
      adc == ADC1 ? RCC_APB2ENR_ADC1EN : (adc == ADC2 ? RCC_APB2ENR_ADC2EN : RCC_APB2ENR_ADC3EN);
  RCC->APB2ENR |= en;
}

void adc_start(adc_reg_t *adc, uint32_t channel) {
  if (channel >= 10) {
    adc->SMPR1 |= ADC_SAMPLE_480 << (3 * (channel % 10));
  } else {
    adc->SMPR2 |= ADC_SAMPLE_480 << (3 * channel);
  }
  nvic_enable_irq(NVIC_IRQ_ADC);
  adc->SQR3 |= channel;
  adc->SQR1 = 0;
  adc->CR1 |= ADC_CR1_EOCIE;
  adc->CR2 |= ADC_CR2_ADON | ADC_CR2_CONT;
  adc->CR2 |= ADC_CR2_SWSTART;
}
