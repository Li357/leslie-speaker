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

// see ST MB1137 for ST Zio connectors to GPIO pins for Nucleo-144 boards

#define ADC_IN3             PIN('A', 3)  // motor speed CV
#define ADC_IN10            PIN('C', 0)  // coil 1 sense
#define ADC_IN13            PIN('C', 3)  // coil 2 sense

#define COIL1_HA            PIN('D', 6)  // these are chosen to be in the same
#define COIL1_LA            PIN('D', 5)  // GPIO bank for easy multiple assignment
#define COIL1_HB            PIN('D', 4)
#define COIL1_LB            PIN('D', 3)

#define COIL2_HA            PIN('E', 10)  // same story here
#define COIL2_LA            PIN('E', 12)
#define COIL2_HB            PIN('E', 14)
#define COIL2_LB            PIN('E', 15)

#define MAX_RPM             (450.0f)
#define STEPS_PER_REV       (200)
#define MICROSTEPS          (16)
#define STEPS               (4 * MICROSTEPS)

#define SPEED_UPDATE_MARGIN (50)
#define COIL_MARGIN         (10)

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
// Both coils start with 0 current, so we need to charge them both up to their setpoints
static bool charging[2] = {true, true};

enum { MOTOR_CV, COIL1_SENSE, COIL2_SENSE };
static uint16_t adcBuf[3] = {0};

int calculateTimerReload(uint32_t adc) {
  return ((float)(ADC_MAX - adc) / ADC_MAX) * (40000 - 2) + 2;
}

void deadtime() {
  asm(".rept 50; nop; .endr");
}

int main() {
  enable_fpu();
  systick_init(SYS_CLOCK / (10E6 / 10));  // count 10us

  // dac_init(DAC_CH1 | DAC_CH2);
  dma_init();
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

  adc_config_channel(ADC1, 3, ADC_28_CYCLES);
  adc_config_channel(ADC1, 10, ADC_28_CYCLES);
  adc_config_channel(ADC1, 13, ADC_28_CYCLES);

  adc_dma_init(ADC1, adcBuf, 3);

  gpio_pin_init((PIN('B', 7)), GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PD);

  tim_init(TIM4);

  GPIO(PINBANK(COIL1_HB))->ODR &= ~(1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
  deadtime();
  GPIO(PINBANK(COIL1_HA))->ODR |= (1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));

  GPIO(PINBANK(COIL2_HB))->ODR &= ~(1 << PINNUM(COIL2_HB) | 1 << PINNUM(COIL2_LA));
  deadtime();
  GPIO(PINBANK(COIL2_HA))->ODR |= (1 << PINNUM(COIL2_HA) | 1 << PINNUM(COIL2_LB));

  // here so that python program can sync and read ADC values correctly aligned to 2-byte boundaries
  char sync[4] = {0x55, 0xBB, 0x55, 0xBB};
  usart_transmit(USART3, sync, 4);

  // ADC DMA must be initialized before ADC is initialized
  adc_start(ADC1);

  while (true) {
    // Motor speed control hysteresis
    /*if (ABS(adcSpeed - lastSpeed) > SPEED_UPDATE_MARGIN) {
      lastSpeed = adcSpeed;
      TIM4->CR1 &= ~1;
      TIM4->ARR = calculateTimerReload(adcSpeed);
      GPIO(1)->ODR ^= (1 << 7);
      TIM4->CR1 |= 1;
    }*/

    usart_transmit(USART3, (char *)&adcBuf[MOTOR_CV], 2);
    usart_transmit(USART3, (char *)&adcBuf[COIL1_SENSE], 2);
    usart_transmit(USART3, (char *)&adcBuf[COIL2_SENSE], 2);

    if (charging[0] && adcBuf[COIL1_SENSE] > setpoints[0] + COIL_MARGIN) {
      GPIO(PINBANK(COIL1_HA))->ODR &= ~(1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
      charging[0] = false;
      deadtime();
      GPIO(PINBANK(COIL1_HB))->ODR |= (1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
    } else if (!charging[0] && adcBuf[COIL1_SENSE] < setpoints[0] - COIL_MARGIN) {
      GPIO(PINBANK(COIL1_HB))->ODR &= ~(1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
      charging[0] = true;
      deadtime();
      GPIO(PINBANK(COIL1_HA))->ODR |= (1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
    }

    if (charging[1] && adcBuf[COIL2_SENSE] > setpoints[1] + COIL_MARGIN) {
      GPIO(PINBANK(COIL2_HA))->ODR &= ~(1 << PINNUM(COIL2_HA) | 1 << PINNUM(COIL2_LB));
      charging[1] = false;
      deadtime();
      GPIO(PINBANK(COIL2_HB))->ODR |= (1 << PINNUM(COIL2_HB) | 1 << PINNUM(COIL2_LA));
    } else if (!charging[1] && adcBuf[COIL1_SENSE] < setpoints[1] - COIL_MARGIN) {
      GPIO(PINBANK(COIL2_HB))->ODR &= ~(1 << PINNUM(COIL2_HB) | 1 << PINNUM(COIL2_LA));
      charging[1] = true;
      deadtime();
      GPIO(PINBANK(COIL2_HA))->ODR |= (1 << PINNUM(COIL2_HA) | 1 << PINNUM(COIL2_LB));
    }
  }

  return 0;
}

void _tim4_handler() {
  if (TIM4->SR & 1) {
    TIM4->SR &= ~1;
    setpoints[0] = SETPOINTS[(index + STEPS / 4) % STEPS];
    setpoints[1] = SETPOINTS[index % STEPS];
    index        = (index + 1) % STEPS;
  }
}

