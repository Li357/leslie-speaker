#include "adc.h"
#include "nvic.h"
#include "rcc.h"

static uint32_t seq = 0;

void adc_init(adc_reg_t *adc) {
  uint32_t en =
      adc == ADC1 ? RCC_APB2ENR_ADC1EN : (adc == ADC2 ? RCC_APB2ENR_ADC2EN : RCC_APB2ENR_ADC3EN);
  RCC->APB2ENR |= en;
  nvic_enable_irq(NVIC_IRQ_ADC);
  adc->CR1 |= ADC_CR1_EOCIE;
  adc->CR2 |= ADC_CR2_EOCS | ADC_CR2_ADON | ADC_CR2_CONT;
}

void adc_config_channel(adc_reg_t *adc, uint32_t channel, adc_sample_rate_t sampleRate) {
  if (channel >= 10) {
    adc->SMPR1 |= sampleRate << (3 * (channel % 10));
  } else {
    adc->SMPR2 |= sampleRate << (3 * channel);
  }

  if (seq < 6) {
    adc->SQR3 |= channel << 5 * seq;
  } else if (seq < 12) {
    adc->SQR2 |= channel << 5 * (seq - 6);
  } else if (seq < 16) {
    adc->SQR1 |= channel << 5 * (seq - 12);
  }
  seq++;
  adc->SQR1 |= seq << ADC_SQR1_L;
}

void adc_start(adc_reg_t *adc) {
  adc->CR2 |= ADC_CR2_SWSTART;
}
