#pragma once

#include "stm32f7.h"

#define ADC_CR1_EOCIE   (1UL << 5)

#define ADC_CR2_ADON    (1UL << 0)
#define ADC_CR2_SWSTART (1UL << 30)
#define ADC_CR2_CONT    (1UL << 1)

#define ADC_SAMPLE_480  (0b111)

typedef volatile struct {
  uint32_t SR;
  uint32_t CR1;
  uint32_t CR2;
  uint32_t SMPR1;
  uint32_t SMPR2;
  uint32_t JOFR1;
  uint32_t JOFR2;
  uint32_t JOFR3;
  uint32_t JOFR4;
  uint32_t HTR;
  uint32_t LTR;
  uint32_t SQR1;
  uint32_t SQR2;
  uint32_t SQR3;
  uint32_t JSQR;
  uint32_t JDR1;
  uint32_t JDR2;
  uint32_t JDR3;
  uint32_t JDR4;
  uint32_t DR;
} adc_reg_t;

typedef volatile struct {
  uint32_t CSR;
  uint32_t CCR;
  uint32_t CDR;
} adc_common_reg_t;

#define ADC1 ((adc_reg_t *)ADC_BASE)
#define ADC2 ((adc_reg_t *)(ADC_BASE + 0x100UL))
#define ADC3 ((adc_reg_t *)(ADC_BASE + 0x200UL))
#define ADC  ((adc_common_reg_t *)(ADC_BASE + 0x300UL))

void adc_init(adc_reg_t *adc);
void adc_start(adc_reg_t *adc, uint32_t channel);
