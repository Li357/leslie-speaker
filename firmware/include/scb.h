#pragma once

#include "stm32f7.h"

#define SCB_SHPR3_SYSTICKSHIFT (24)
#define SCB_SHPR3_PENDSVSHIFT  (16)
#define SCB_ICSR_PENDSVSET     (1UL << 28)
#define SCB_CPACR_CP10         (20)
#define SCB_CPACR_CP11         (22)
#define SCB_CPACR_FULLPRIV     (0b11)

typedef struct {
  uint32_t CPUID;
  uint32_t ICSR;
  uint32_t VTOR;
  uint32_t AIRCR;
  uint32_t SCR;
  uint32_t CCR;
  uint32_t SHPR1;
  uint32_t SHPR2;
  uint32_t SHPR3;
  uint32_t SHCRS;
  uint32_t CFSR;
  uint32_t HFSR;
  uint32_t DFSR;
  uint32_t MMFAR;
  uint32_t BFAR;
  uint32_t AFSR;
  uint32_t _RESERVED0[18];
  uint32_t CPACR;
  uint32_t _RESERVED1;
} scb_reg_t;

#define SCB ((scb_reg_t *)SCB_BASE)

void enable_fpu();
