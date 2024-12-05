#pragma once

#include "gpio.h"

/*
 * The STM32F756ZG has a max frequency of 216MHz, with
 * PLL_P, PLL_N, and PLL_M to scale the VCO:
 *
 * HSI    = 16MHz
 * f_VCOi = HSI / PLL_M <= 2MHz * f_VCOo = f_VCOi * PLL_N with 100MHz <= f_VCOo
 * <= 432MHz f_sys  = f_VCOo / PLL_P <= 216MHz with PLL_P = 2, 4, 6, 8
 *
 * We'll target 100MHz = HSI / 16 * 100 / 2 for now, at 3.3V giving a latency
 * of 3 wait states
 */

#define PLL_M         (8)
#define PLL_N         (100)
#define PLL_P         (0)  // 2
#define PLL_P_ACTUAL  (2)
#define SYS_CLOCK     (BASE_CLOCK / PLL_M * PLL_N / PLL_P_ACTUAL)
#define FLASH_LATENCY (3)

/*
 * We also have scalers for the AHB1/2, APB1/2 clocks:
 * - AHB1/2 <= 216MHz, APB2 <= 108MHz, APB1 <= 54MHz
 */

#define APB1_PRE   (0b101)
#define APB2_PRE   (0b100)
#define AHB_PRE    (0)
#define APB1_CLOCK (SYS_CLOCK / 4)
#define APB2_CLOCK (SYS_CLOCK / 2)
#define AHB_CLOCK  SYS_CLOCK
