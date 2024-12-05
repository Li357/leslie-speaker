#include "adc.h"
#include "dma.h"
#include "nvic.h"
#include "rcc.h"

static uint32_t seq = 0;

void adc_init(adc_reg_t *adc) {
  uint32_t en =
      adc == ADC1 ? RCC_APB2ENR_ADC1EN : (adc == ADC2 ? RCC_APB2ENR_ADC2EN : RCC_APB2ENR_ADC3EN);
  RCC->APB2ENR |= en;
  // nvic_enable_irq(NVIC_IRQ_ADC);
  // adc->CR1 |= ADC_CR1_EOCIE;
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

void dma_init() {
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

  DMA2->streams[0].CR |= (0b10 << DMA_SxCR_PL_SHIFT | 0b01 << DMA_SxCR_MSIZE_SHIFT |
                          0b01 << DMA_SxCR_PSIZE_SHIFT | 0b00 << DMA_SxCR_CHSEL_SHIFT |
                          0b00 << DMA_SxCR_DIR_SHIFT | DMA_SxCR_MINC | DMA_SxCR_CIRC | (1UL << 5));
}

void adc_dma_init(adc_reg_t *adc, uint16_t *ptr, uint16_t len) {
  DMA2->streams[0].M0AR = (uint32_t)ptr;
  DMA2->streams[0].PAR  = (uint32_t)&adc->DR;
  DMA2->streams[0].NDTR = len;
  DMA2->streams[0].CR |= DMA_SxCR_EN;

  adc->CR1 |= (1UL << 8);
  adc->CR2 |= ADC_CR2_DDS | ADC_CR2_DMA;
}

