#include "scb.h"

void enable_fpu() {
  SCB->CPACR |= (SCB_CPACR_FULLPRIV << SCB_CPACR_CP10) | (SCB_CPACR_FULLPRIV << SCB_CPACR_CP11);
}
