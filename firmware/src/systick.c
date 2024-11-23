#include "systick.h"
#include <stdbool.h>
#include "rcc.h"
#include "scb.h"

static volatile uint32_t _systicks;

void systick_init(uint32_t ticks) {
  // Enable SYSCFG for systick
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  // SysTick is 24-bit
  SYST->RVR = (ticks - 1) & 0xFFFFFFUL;
  SYST->CVR = 0;
  SYST->CSR |= SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;
  SCB->SHPR3 = 0;  // set systick priority to highest
}

void delay(uint32_t ticks) {
  SYST->CSR |= SYST_CSR_ENABLE;
  _systicks = 0;
  while (_systicks < ticks) {}
  SYST->CSR &= ~SYST_CSR_ENABLE;
}

void _systick_handler() {
  _systicks++;
}

