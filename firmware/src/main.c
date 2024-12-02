#include <stdbool.h>
#include "adc.h"
#include "dac.h"
#include "gpio.h"
#include "scb.h"
#include "stm32f7.h"
#include "systick.h"
#include "tim.h"
#include "util.h"
#define ADC_IN3             PIN('A', 3)  // A0 motor speed CV
#define ADC_IN10            PIN('C', 0)  // A1 coil 1 sense
#define ADC_IN13            PIN('C', 3)  // A2 coil 2 sense

#define COIL1_HALB          PIN('D', 6)  // D52 coil 1 HA, LB logic
#define COIL1_HBLA          PIN('D', 5)  // D53 coil 1 HB, LA logic
#define COIL2_HALB          PIN('D', 4)  // D54
#define COIL2_HBLA          PIN('D', 3)  // D55

#define MAX_RPM             (450.0f)
#define STEPS_PER_REV       (200)
#define MICROSTEPS          (16)
#define STEPS               (4 * MICROSTEPS)

#define SPEED_UPDATE_MARGIN (50)

static uint32_t SETPOINTS[STEPS] = {
    4095, 4085, 4055, 4006, 3939, 3853, 3749, 3630, 3495, 3346, 3185, 3012, 2831, 2641, 2446, 2248,
    2047, 1846, 1648, 1453, 1263, 1082, 909,  748,  599,  464,  345,  241,  155,  88,   39,   9,
    0,    9,    39,   88,   155,  241,  345,  464,  599,  748,  909,  1082, 1263, 1453, 1648, 1846,
    2047, 2248, 2446, 2641, 2831, 3012, 3185, 3346, 3495, 3630, 3749, 3853, 3939, 4006, 4055, 4085,
};

static uint32_t index        = 0;
static uint32_t setpoints[2] = {3999, 0};
// static uint16_t lastSpeed    = 0;
static uint16_t adcSpeed = 0;

int calculateTimerReload(uint32_t adc) {
  return ((float)(ADC_MAX - adc) / ADC_MAX) * (40000 - 2) + 2;
}

static int curr      = 0;
static bool charging = true;

int main() {
  enable_fpu();
  systick_init(BASE_CLOCK / 1E5);  // count 10us, systick based on HSI base clock

  dac_init(DAC_CH1 | DAC_CH2);
  adc_init(ADC1);
  delay(10);  // ADC needs some calibration time

  gpio_pin_init(ADC_IN3, GPIO_ANALOG, 0, 0, 0);
  gpio_pin_init(ADC_IN10, GPIO_ANALOG, 0, 0, 0);
  gpio_pin_init(ADC_IN13, GPIO_ANALOG, 0, 0, 0);

  gpio_pin_init(COIL1_HALB, GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_HIGHSPEED, GPIO_PD);
  gpio_pin_init(COIL1_HBLA, GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_HIGHSPEED, GPIO_PD);

  gpio_pin_init(PIN('A', 4), GPIO_ANALOG, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PU);
  gpio_pin_init(PIN('A', 5), GPIO_ANALOG, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PU);

  adc_config_channel(ADC1, 3, ADC_480_CYCLES);
  adc_config_channel(ADC1, 10, ADC_28_CYCLES);
  adc_config_channel(ADC1, 13, ADC_28_CYCLES);

  gpio_pin_init((PIN('B', 7)), GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PD);

  adc_start(ADC1);

  tim_init(TIM4);
  GPIO(PINBANK(COIL1_HBLA))->ODR &= ~(1 << PINNUM(COIL1_HBLA));
  GPIO(PINBANK(COIL1_HALB))->ODR |= 1 << PINNUM(COIL1_HALB);

  while (true) {
    // Motor speed control hysteresis
    /*if (ABS(adcSpeed - lastSpeed) > SPEED_UPDATE_MARGIN) {
      lastSpeed = adcSpeed;
      TIM4->CR1 &= ~1;
      TIM4->ARR = calculateTimerReload(adcSpeed);
      GPIO(1)->ODR ^= (1 << 7);
      TIM4->CR1 |= 1;
    }*/

    // if current increasing above setpoint
    if (charging && curr > setpoints[0] + 10) {
      GPIO(PINBANK(COIL1_HALB))->ODR &= ~(1 << PINNUM(COIL1_HALB));
      charging = false;
      GPIO(PINBANK(COIL1_HBLA))->ODR |= 1 << PINNUM(COIL1_HBLA);
    } else if (!charging && curr < setpoints[0] - 10) {
      GPIO(PINBANK(COIL1_HBLA))->ODR &= ~(1 << PINNUM(COIL1_HBLA));
      charging = true;
      GPIO(PINBANK(COIL1_HALB))->ODR |= 1 << PINNUM(COIL1_HALB);
    }

    // delay(10);
    /*for (int i = 0; i < STEPS; i++) {
      // current in coil two lags by quarter period
      setpoints[0] = SETPOINTS[(i + STEPS / 4) % STEPS];
      setpoints[1] = SETPOINTS[i];
      dac_set(setpoints[0], setpoints[1]);
      delay(speedDelay);
    }*/
  }

  return 0;
}

void _tim4_handler() {
  if (TIM4->SR & 1) {
    TIM4->SR &= ~1;
    setpoints[0] = SETPOINTS[(index + STEPS / 4) & 0x3F];
    setpoints[1] = SETPOINTS[index];
    index        = (index + 1) & 0x3F;
  }
  /*if (charging) {
    curr++;
  } else {
    curr--;
  }*/
}

void _adc_handler() {
  adcSpeed = ADC1->DR;
}

void _tim3_handler() {
  if (TIM3->SR & 1) {
    TIM3->SR &= ~1;
    if (charging) {
      curr += 10;
    } else {
      curr -= 10;
    }
    dac_set(curr > 4095 ? 4095 : (curr < 0 ? 0 : curr), 0);
  }
}
