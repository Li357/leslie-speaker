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
#define COIL_MARGIN         (2)

/*static uint32_t SETPOINTS[STEPS] = {
    4095, 4085, 4055, 4006, 3939, 3853, 3749, 3630, 3495, 3346, 3185, 3012, 2831, 2641, 2446, 2248,
    2047, 1846, 1648, 1453, 1263, 1082, 909,  748,  599,  464,  345,  241,  155,  88,   39,   9,
    0,    9,    39,   88,   155,  241,  345,  464,  599,  748,  909,  1082, 1263, 1453, 1648, 1846,
    2047, 2248, 2446, 2641, 2831, 3012, 3185, 3346, 3495, 3630, 3749, 3853, 3939, 4006, 4055, 4085,
};*/

/*static uint32_t SETPOINTS[STEPS] = {185, 184, 183, 181, 177, 174, 169, 164, 157, 151, 143, 136,
   127, 119, 110, 101, 92,  83,  74,  65,  57,  48,  41,  33,  27,  20, 15,  10,  7,   3,   1,   0,
   0,   0,   1,   3,   7,   10,  15, 20,  27,  33,  41,  48,  57,  65,  74,  83,  92,  101, 110,
   119, 127, 136, 143, 151, 157, 164, 169, 174, 177, 181, 183, 184};*/

/*static uint32_t SETPOINTS[STEPS] = {
    2500,  2487,  2451,  2392,  2309,  2204,  2078,  1932,  1767,  1585,  1388,  1178,  956,
    725,   487,   245,   0,     -245,  -487,  -725,  -956,  -1178, -1388, -1585, -1767, -1932,
    -2078, -2204, -2309, -2392, -2451, -2487, -2500, -2487, -2451, -2392, -2309, -2204, -2078,
    -1932, -1767, -1585, -1388, -1178, -956,  -725,  -487,  -245,  0,     245,   487,   725,
    956,   1178,  1388,  1585,  1767,  1932,  2078,  2204,  2309,  2392,  2451,  2487};*/
static uint32_t SETPOINTS[STEPS] = {
    2600, 2593, 2575, 2546, 2504, 2452, 2389, 2316, 2233, 2142, 2044, 1939, 1828, 1712, 1593, 1472,
    1350, 1227, 1106, 987,  871,  760,  655,  557,  466,  383,  310,  247,  195,  153,  124,  106,
    100,  106,  124,  153,  195,  247,  310,  383,  466,  557,  655,  760,  871,  987,  1106, 1227,
    1349, 1472, 1593, 1712, 1828, 1939, 2044, 2142, 2233, 2316, 2389, 2452, 2504, 2546, 2575, 2593};

static uint32_t STATES[STEPS] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
};

static uint32_t lastSpeed = 0;

// static volatile int index    = 0;
static uint32_t setpoints[2] = {0, 0};
// Both coils start with 0 current, so we need to charge them both up to their setpoints
// static bool charging[2] = {true, true};
// static bool crossed[2]  = {false, false};
static int states[2]   = {0, 0};
static bool handled[2] = {false, false};

enum { MOTOR_CV, COIL1_SENSE, COIL2_SENSE };
static uint16_t adcBuf[3] = {0};

typedef enum { STATE0, STATE1, STATE2, STATE3 } State;

int calculateTimerReload(uint32_t adc) {
  return ((float)(ADC_MAX - adc) / ADC_MAX) * (200 - 14) + 14;
}

void deadtime() {
  asm(".rept 10; nop; .endr");
}

int main() {
  enable_fpu();
  systick_init(SYS_CLOCK / (10E6 / 10));  // count 10us

  dac_init(DAC_CH1 | DAC_CH2);
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

  gpio_pin_init(COIL2_HA, GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_VHIGHSPEED, GPIO_PD);
  gpio_pin_init(COIL2_LB, GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_VHIGHSPEED, GPIO_PD);
  gpio_pin_init(COIL2_HB, GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_VHIGHSPEED, GPIO_PD);
  gpio_pin_init(COIL2_LA, GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_VHIGHSPEED, GPIO_PD);

  gpio_pin_init(PIN('A', 4), GPIO_ANALOG, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PU);
  gpio_pin_init(PIN('A', 5), GPIO_ANALOG, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PU);

  adc_config_channel(ADC1, 3, ADC_28_CYCLES);
  adc_config_channel(ADC1, 10, ADC_28_CYCLES);
  adc_config_channel(ADC1, 13, ADC_28_CYCLES);

  adc_dma_init(ADC1, adcBuf, 3);

  gpio_pin_init((PIN('B', 7)), GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_MEDSPEED, GPIO_PD);
  gpio_pin_init(PIN('E', 2), GPIO_OUTPUT, GPIO_PUSHPULL, GPIO_VHIGHSPEED, GPIO_PD);

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

  // SYST->CSR |= SYST_CSR_ENABLE;

  // char *end = "\n";

  while (true) {
    // Motor speed control hysteresis
    if (ABS(adcBuf[MOTOR_CV] - lastSpeed) > SPEED_UPDATE_MARGIN) {
      lastSpeed = adcBuf[MOTOR_CV];
      TIM4->CR1 &= ~1;
      TIM4->ARR = calculateTimerReload(adcBuf[MOTOR_CV]);
      // GPIO(1)->ODR ^= (1 << 7);
      TIM4->CR1 |= 1;
    }

    // usart_transmit(USART3, (char *)&setpoints[0], 4);
    // usart_transmit(USART3, (char *)&adcBuf[MOTOR_CV], 2);
    // usart_transmit(USART3, (char *)&adcBuf[COIL1_SENSE], 2);
    // usart_transmit(USART3, (char *)&adcBuf[COIL2_SENSE], 2);

    /*if (charging[0] && adcBuf[COIL1_SENSE] > setpoints[0] + COIL_MARGIN) {
      GPIO(PINBANK(COIL1_HA))->ODR &= ~(1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
      charging[0] = false;
      deadtime();
      GPIO(PINBANK(COIL1_HB))->ODR |= (1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
    } else if (!charging[0] && adcBuf[COIL1_SENSE] < setpoints[0] - COIL_MARGIN) {
      GPIO(PINBANK(COIL1_HB))->ODR &= ~(1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
      crossed[0]  = false;
      charging[0] = true;
      deadtime();
      GPIO(PINBANK(COIL1_HA))->ODR |= (1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
    }*/

    if (!handled[0]) {
      if (states[0] == STATE0) {
        GPIO(PINBANK(COIL1_LB))->ODR &= ~(1 << PINNUM(COIL1_LB) | 1 << PINNUM(COIL1_LA));
        deadtime();
        GPIO(PINBANK(COIL1_HA))->ODR |= (1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
      } else if (states[0] == STATE1) {
        GPIO(PINBANK(COIL1_HA))->ODR &= ~(1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
        deadtime();
        GPIO(PINBANK(COIL1_LA))->ODR |= (1 << PINNUM(COIL1_LA) | 1 << PINNUM(COIL1_LB));
      } else if (states[0] == STATE2) {
        GPIO(PINBANK(COIL1_LA))->ODR &= ~(1 << PINNUM(COIL1_LA) | 1 << PINNUM(COIL1_LB));
        deadtime();
        GPIO(PINBANK(COIL1_HB))->ODR |= (1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
      } else if (states[0] == STATE3) {
        GPIO(PINBANK(COIL1_HB))->ODR &= ~(1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
        deadtime();
        GPIO(PINBANK(COIL1_LB))->ODR |= (1 << PINNUM(COIL1_LA) | 1 << PINNUM(COIL1_LB));
      }
      handled[0] = true;
    }

    if (!handled[1]) {
      if (states[1] == STATE0) {
        GPIO(PINBANK(COIL2_LB))->ODR &= ~(1 << PINNUM(COIL2_LB) | 1 << PINNUM(COIL2_LA));
        deadtime();
        GPIO(PINBANK(COIL2_HA))->ODR |= (1 << PINNUM(COIL2_HA) | 1 << PINNUM(COIL2_LB));
      } else if (states[1] == STATE1) {
        GPIO(PINBANK(COIL2_HA))->ODR &= ~(1 << PINNUM(COIL2_HA) | 1 << PINNUM(COIL2_LB));
        deadtime();
        GPIO(PINBANK(COIL2_LA))->ODR |= (1 << PINNUM(COIL2_LA) | 1 << PINNUM(COIL2_LB));
      } else if (states[1] == STATE2) {
        GPIO(PINBANK(COIL2_LA))->ODR &= ~(1 << PINNUM(COIL2_LA) | 1 << PINNUM(COIL2_LB));
        deadtime();
        GPIO(PINBANK(COIL2_HB))->ODR |= (1 << PINNUM(COIL2_HB) | 1 << PINNUM(COIL2_LA));
      } else if (states[1] == STATE3) {
        GPIO(PINBANK(COIL2_HB))->ODR &= ~(1 << PINNUM(COIL2_HB) | 1 << PINNUM(COIL2_LA));
        deadtime();
        GPIO(PINBANK(COIL2_LB))->ODR |= (1 << PINNUM(COIL2_LA) | 1 << PINNUM(COIL2_LB));
      }
      handled[1] = true;
    }

    /*
        if (setpoints[0] >= 0) {
          if (charging[0] && adcBuf[COIL1_SENSE] > setpoints[0] + COIL_MARGIN) {
            GPIO(PINBANK(COIL1_HA))->ODR &= ~(1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
            charging[0] = false;
            deadtime();
            GPIO(PINBANK(COIL1_HB))->ODR |= (1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
          } else if (!charging[0] && (crossed[0] || adcBuf[COIL1_SENSE] < setpoints[0] -
       COIL_MARGIN)) { GPIO(PINBANK(COIL1_HB))->ODR &= ~(1 << PINNUM(COIL1_HB) | 1 <<
       PINNUM(COIL1_LA)); crossed[0]  = false; charging[0] = true; deadtime();
            GPIO(PINBANK(COIL1_HA))->ODR |= (1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
          }
        } else {
          if (!charging[0] && adcBuf[COIL1_SENSE] > ABS(setpoints[0]) + COIL_MARGIN) {
            GPIO(PINBANK(COIL1_HB))->ODR &= ~(1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
            charging[0] = true;
            deadtime();
            GPIO(PINBANK(COIL1_HA))->ODR |= (1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
          } else if (charging[0] &&
                     (!crossed[0] || adcBuf[COIL1_SENSE] < ABS(setpoints[0]) - COIL_MARGIN)) {
            GPIO(PINBANK(COIL1_HA))->ODR &= ~(1 << PINNUM(COIL1_HA) | 1 << PINNUM(COIL1_LB));
            charging[0] = false;
            crossed[0]  = true;
            deadtime();
            GPIO(PINBANK(COIL1_HB))->ODR |= (1 << PINNUM(COIL1_HB) | 1 << PINNUM(COIL1_LA));
          }
        }

        if (setpoints[1] >= 0) {
          if (charging[1] && adcBuf[COIL2_SENSE] > setpoints[1] + COIL_MARGIN) {
            GPIO(PINBANK(COIL2_HA))->ODR &= ~(1 << PINNUM(COIL2_HA) | 1 << PINNUM(COIL2_LB));
            charging[1] = false;
            deadtime();
            GPIO(PINBANK(COIL2_HB))->ODR |= (1 << PINNUM(COIL2_HB) | 1 << PINNUM(COIL2_LA));
          } else if (!charging[1] && (crossed[1] || adcBuf[COIL2_SENSE] < setpoints[1] -
       COIL_MARGIN)) { GPIO(PINBANK(COIL2_HB))->ODR &= ~(1 << PINNUM(COIL2_HB) | 1 <<
       PINNUM(COIL2_LA)); crossed[1]  = false; charging[1] = true; deadtime();
            GPIO(PINBANK(COIL2_HA))->ODR |= (1 << PINNUM(COIL2_HA) | 1 << PINNUM(COIL2_LB));
          }
        } else {
          if (!charging[1] && adcBuf[COIL2_SENSE] > ABS(setpoints[1]) + COIL_MARGIN) {
            GPIO(PINBANK(COIL2_HB))->ODR &= ~(1 << PINNUM(COIL2_HB) | 1 << PINNUM(COIL2_LA));
            charging[1] = true;
            deadtime();
            GPIO(PINBANK(COIL2_HA))->ODR |= (1 << PINNUM(COIL2_HA) | 1 << PINNUM(COIL2_LB));
          } else if (charging[1] &&
                     (!crossed[1] || adcBuf[COIL2_SENSE] < ABS(setpoints[1]) - COIL_MARGIN)) {
            GPIO(PINBANK(COIL2_HA))->ODR &= ~(1 << PINNUM(COIL2_HA) | 1 << PINNUM(COIL2_LB));
            charging[1] = false;
            crossed[1]  = true;
            deadtime();
            GPIO(PINBANK(COIL2_HB))->ODR |= (1 << PINNUM(COIL2_HB) | 1 << PINNUM(COIL2_LA));
          }
        }*/
  }

  return 0;
}

static int i = 0;
void _tim4_handler() {
  if (TIM4->SR & 1) {
    // asm("cpsie i");
    TIM4->SR &= ~1;
    setpoints[0] = SETPOINTS[(i + 16) % 64];
    setpoints[1] = SETPOINTS[i];

    if (states[0] != STATES[(i + 16) % 64]) handled[0] = false;
    if (states[1] != STATES[i]) handled[1] = false;

    states[0] = STATES[(i + 16) % 64];
    states[1] = STATES[i];
    i         = (i + 1) % 64;
    // GPIO(PINBANK(COIL2_HA))->ODR ^= (1 << PINNUM(COIL2_HA));
    // dac_set(setpoints[0] + 1400, setpoints[1] + 1400);
    // usart_transmit(USART3, (char*)&cnt, 2);
    // usart_transmit(USART3, end, 1);
    // asm("cpsid i");
  }
}
