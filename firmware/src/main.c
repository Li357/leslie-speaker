#include <stdbool.h>
#include "adc.h"
#include "dac.h"
#include "gpio.h"
#include "scb.h"
#include "stm32f7.h"
#include "systick.h"
#include "tim.h"
#include "usart.h"
#include "util.h"

#define ADC_IN3  PIN('A', 3)  // A0 motor speed CV
#define ADC_IN10 PIN('C', 0)  // A1 coil 1 sense
#define ADC_IN13 PIN('C', 3)  // A2 coil 2 sense

#define COIL1_HA PIN('D', 6)  // D52 coil 1 HA, LB logic
#define COIL1_LA PIN('D', 5)
#define COIL1_HB PIN('D', 4)
#define COIL1_LB PIN('D', 3)  // D53 coil 1 HB, LA logic
// #define COIL2_HALB          PIN('D', 4)  // D54
// #define COIL2_HBLA          PIN('D', 3)  // D55

#define MAX_RPM             (450.0f)
#define STEPS_PER_REV       (200)
#define MICROSTEPS          (16)
#define STEPS               (4 * MICROSTEPS)

#define SPEED_UPDATE_MARGIN (50)

/*static uint32_t SETPOINTS[STEPS] = {
    4095, 4085, 4055, 4006, 3939, 3853, 3749, 3630, 3495, 3346, 3185, 3012, 2831, 2641, 2446, 2248,
    2047, 1846, 1648, 1453, 1263, 1082, 909,  748,  599,  464,  345,  241,  155,  88,   39,   9,
    0,    9,    39,   88,   155,  241,  345,  464,  599,  748,  909,  1082, 1263, 1453, 1648, 1846,
    2047, 2248, 2446, 2641, 2831, 3012, 3185, 3346, 3495, 3630, 3749, 3853, 3939, 4006, 4055, 4085,
};*/

static uint32_t SETPOINTS[STEPS] = {185, 184, 183, 181, 177, 174, 169, 164, 157, 151, 143, 136, 127,
                                    119, 110, 101, 92,  83,  74,  65,  57,  48,  41,  33,  27,  20,
                                    15,  10,  7,   3,   1,   0,   0,   0,   1,   3,   7,   10,  15,
                                    20,  27,  33,  41,  48,  57,  65,  74,  83,  92,  101, 110, 119,
                                    127, 136, 143, 151, 157, 164, 169, 174, 177, 181, 183, 184};
static uint32_t index            = 0;
static uint32_t setpoints[2]     = {0, 0};
static uint32_t adcSpeed         = 0;

int calculateTimerReload(uint32_t adc) {
  return ((float)(ADC_MAX - adc) / ADC_MAX) * (40000 - 2) + 2;
}

static bool charging = true;

void deadtime() {
  asm(".rept 50 ; nop ; .endr");
}

int main() {
  enable_fpu();
  systick_init(SYS_CLOCK / 100000);  // count 10us

  // dac_init(DAC_CH1 | DAC_CH2);
  adc_init(ADC1);
  delay(10);  // ADC needs some calibration time

  usart_init(USART3, 115200);

  gpio_pin_init(ADC_IN3, GPIO_ANALOG, 0, 0, 0);
  gpio_pin_init(ADC_IN10, GPIO_ANALOG, 0, 0, 0);
  gpio_pin_init(ADC_IN13, GPIO_ANALOG, 0, 0, 0);

  gpio_pin_init(COIL1_HA, GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_VHIGHSPEED, GPIO_PD);
  gpio_pin_init(COIL1_LB, GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_VHIGHSPEED, GPIO_PD);
  gpio_pin_init(COIL1_HB, GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_VHIGHSPEED, GPIO_PD);
  gpio_pin_init(COIL1_LA, GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_VHIGHSPEED, GPIO_PD);

  gpio_pin_init(PIN('A', 4), GPIO_ANALOG, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PU);
  gpio_pin_init(PIN('A', 5), GPIO_ANALOG, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PU);

  adc_config_channel(ADC1, 3, ADC_480_CYCLES);
  adc_config_channel(ADC1, 10, ADC_28_CYCLES);
  adc_config_channel(ADC1, 13, ADC_28_CYCLES);

  gpio_pin_init((PIN('B', 7)), GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PD);

  adc_start(ADC1);

  tim_init(TIM4);
  GPIO(PINBANK(COIL1_HB))->ODR &= ~(1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
  deadtime();
  GPIO(PINBANK(COIL1_HA))->ODR |= (1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
  /*
  GPIO(PINBANK(COIL1_HA))->ODR &= ~(1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
  deadtime();
  GPIO(PINBANK(COIL1_HB))->ODR |= (1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));

  GPIO(PINBANK(COIL1_HB))->ODR &= ~(1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
  deadtime();
  GPIO(PINBANK(COIL1_HA))->ODR |= (1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));*/

  char sync[4] = {0x55, 0xBB, 0x55, 0xBB};
  usart_transmit(USART3, sync, 4);

  while (true) {
    // Motor speed control hysteresis
    /*if (ABS(adcSpeed - lastSpeed) > SPEED_UPDATE_MARGIN) {
      lastSpeed = adcSpeed;
      TIM4->CR1 &= ~1;
      TIM4->ARR = calculateTimerReload(adcSpeed);
      GPIO(1)->ODR ^= (1 << 7);
      TIM4->CR1 |= 1;
    }*/

    usart_transmit(USART3, (char *)&adcSpeed, 4);
    //   usart_transmit(USART3, txt, 5);

    // if current increasing above setpoint
    if (charging && adcSpeed > setpoints[0] + 10) {
      GPIO(PINBANK(COIL1_HA))->ODR &= ~(1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
      charging = false;
      deadtime();
      GPIO(PINBANK(COIL1_HB))->ODR |= (1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
    } else if (!charging && adcSpeed < setpoints[0] - 10) {
      GPIO(PINBANK(COIL1_HB))->ODR &= ~(1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
      charging = true;
      deadtime();
      GPIO(PINBANK(COIL1_HA))->ODR |= (1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
    }
  }

  return 0;
}

void _tim4_handler() {
  if (TIM4->SR & 1) {
    TIM4->SR &= ~1;
    setpoints[0] = SETPOINTS[(index + STEPS / 4) & 0x3F];
    setpoints[1] = SETPOINTS[index];
    index        = (index + 1) & 0x3F;
    // usart_transmit(USART3, test, 4);
    // dac_set(setpoints[0], setpoints[1]);
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

/*void _tim3_handler() {
  if (TIM3->SR & 1) {
    TIM3->SR &= ~1;
    if (charging) {
      curr += 10;
    } else {
      curr -= 10;
    }
    dac_set(curr > 4095 ? 4095 : (curr < 0 ? 0 : curr), 0);
  }
}*/
