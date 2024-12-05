#include "tim.h"
#include "config.h"
#include "nvic.h"
#include "rcc.h"

void tim_adv_pwm_init(tim_adv_reg_t *tim, uint32_t freq, float duty) {
  RCC->APB2ENR |= 1;
  tim->PSC  = 15;
  tim->ARR  = 1000;
  tim->CCR1 = 500;

  // tim->CCMR1 |= (0b110 << 4) | (1 << 3) | (0b110 << 12) | (1 << 11);
  // tim->CR1 |= (1 << 7);
  //  tim->CNT = 0;
  // tim->CCER |= 1 | (1 << 4) | (1 << 2);
  // tim->BDTR |= 1 << 15;
  //  tim->EGR |= 1;
}

void tim_adv_pwm_start(tim_adv_reg_t *tim) {
  tim->CR1 |= 1;
}

void tim_init(tim_reg_t *tim) {
  //  TIM3->CR1 &= ~1;
  TIM4->CR1 &= ~1;
  RCC->APB1ENR |= (1 << 2) | (1 << 1);

  nvic_set_priority(NVIC_IRQ_TIM4, 1);
  nvic_enable_irq(NVIC_IRQ_TIM4);
  // nvic_set_priority(NVIC_IRQ_TIM3, 3);
  // nvic_enable_irq(NVIC_IRQ_TIM3);

  RCC->APB1RSTR |= (1 << 2) | (1 << 1);
  RCC->APB1RSTR &= ~((1 << 2) | (1 << 1));

  // TIM3->CR1 |= (1 << 7);
  TIM4->CR1 |= (1 << 7);
  TIM4->PSC = 9;
  TIM4->ARR = 999;  // 1
  TIM4->EGR |= 1;
  TIM4->DIER |= 1;
  TIM4->CR1 |= 1;

  /*
  TIM3->PSC = 19;
  TIM3->ARR = 19;
  TIM3->EGR |= 1;
  TIM3->DIER |= 1;
  TIM3->CR1 |= 1;*/
}
