#pragma once

#include "stm32f7.h"

typedef volatile struct {
  uint32_t CR1;
  uint32_t CR2;
  uint32_t SMCR;
  uint32_t DIER;
  uint32_t SR;
  uint32_t EGR;
  uint32_t CCMR1;
  uint32_t CCMR2;
  uint32_t CCER;
  uint32_t CNT;
  uint32_t PSC;
  uint32_t ARR;
  uint32_t RCR;
  uint32_t CCR1;
  uint32_t CCR2;
  uint32_t CCR3;
  uint32_t CCR4;
  uint32_t BDTR;
  uint32_t DCR;
  uint32_t DMAR;
  uint32_t CCMR3;
  uint32_t CCR5;
  uint32_t CCR6;
} tim_adv_reg_t;

typedef volatile struct {
  uint32_t CR1;
  uint32_t CR2;
  uint32_t SMCR;
  uint32_t DIER;
  uint32_t SR;
  uint32_t EGR;
  uint32_t CCMR1;
  uint32_t CCMR2;
  uint32_t CCER;
  uint32_t CNT;
  uint32_t PSC;
  uint32_t ARR;
  uint32_t _RESERVED0;
  uint32_t CCR1;
  uint32_t CCR2;
  uint32_t CCR3;
  uint32_t CCR4;
  uint32_t _RESERVED1;
  uint32_t DCR;
  uint32_t DMAR;
  uint32_t OR;
} tim_reg_t;

#define TIM1 ((tim_adv_reg_t *)TIM1_BASE)
#define TIM8 ((tim_adv_reg_t *)TIM8_BASE)
#define TIM3 ((tim_reg_t *)TIM3_BASE)
#define TIM4 ((tim_reg_t *)TIM4_BASE)

void tim_adv_pwm_init(tim_adv_reg_t *tim, uint32_t freq, float duty);
void tim_adv_pwm_start(tim_adv_reg_t *tim);
void tim_init(tim_reg_t *tim);
