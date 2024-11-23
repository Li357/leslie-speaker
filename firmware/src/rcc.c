#include "rcc.h"
#include "config.h"
#include "flash.h"

void _system_init() {
  // Enable flash instruction prefetch and set latency
  FLASH->ACR |= FLASH_ACR_PRFTEN | FLASH_LATENCY;

  // Set PLL factors from config.h
  RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLP | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLM);
  RCC->PLLCFGR |= PLL_P << RCC_PLLCFGR_PLLPSHIFT;
  RCC->PLLCFGR |= PLL_N << RCC_PLLCFGR_PLLNSHIFT;
  RCC->PLLCFGR |= PLL_M << RCC_PLLCFGR_PLLMSHIFT;
  RCC->CR |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY)) {}

  // Set peripheral bus prescalers
  RCC->CFGR &= ~(RCC_CFGR_PPRE2 | RCC_CFGR_PPRE1 | RCC_CFGR_HPRE);
  RCC->CFGR |= APB1_PRE << RCC_CFGR_PPRE1SHIFT;
  RCC->CFGR |= APB2_PRE << RCC_CFGR_PPRE2SHIFT;
  RCC->CFGR |= AHB_PRE << RCC_CFGR_HPRESHIFT;
}
