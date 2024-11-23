#include <stdbool.h>
#include "config.h"
#include "dac.h"
#include "gpio.h"
#include "systick.h"

static uint32_t values[64] = {
    4095, 4085, 4055, 4006, 3939, 3853, 3749, 3630, 3495, 3346, 3185, 3012, 2831, 2641, 2446, 2248,
    2047, 1846, 1648, 1453, 1263, 1082, 909,  748,  599,  464,  345,  241,  155,  88,   39,   9,
    0,    9,    39,   88,   155,  241,  345,  464,  599,  748,  909,  1082, 1263, 1453, 1648, 1846,
    2047, 2248, 2446, 2641, 2831, 3012, 3185, 3346, 3495, 3630, 3749, 3853, 3939, 4006, 4055, 4085,
};

int main() {
  systick_init(BASE_CLOCK / 1000);  // count milliseconds, systick based on HSI base clock

  dac_init(DAC_CH1 | DAC_CH2);
  delay(10);  // DAC needs some wakeup time

  gpio_pin_init(DAC_OUT1, GPIO_ANALOG, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PU);
  gpio_pin_init(DAC_OUT2, GPIO_ANALOG, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PU);

  while (true) {
    for (int i = 0; i < 64; i++) {
      dac_set(values[i], values[(i + 16) % 64]);
      delay(50);
    }
  }

  return 0;
}
