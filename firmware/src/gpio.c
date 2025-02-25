#include "gpio.h"
#include "rcc.h"

void gpio_pin_init(gpio_pin_t pin, gpio_mode_t mode, gpio_output_type_t type,
                   gpio_output_speed_t speed, gpio_pupd_t pupd) {
  // Enable the GPIO bank in AHB
  RCC->AHB1ENR |= 1 << PINBANK(pin);

  gpio_reg_t *GPIOx = GPIO(PINBANK(pin));
  uint32_t pos      = PINNUM(pin) << 1;
  GPIOx->MODER &= ~(0x3UL << pos);
  GPIOx->MODER |= mode << pos;

  GPIOx->OTYPER &= ~(0x1UL << PINNUM(pin));
  GPIOx->OTYPER |= type << PINNUM(pin);

  GPIOx->OSPEEDR &= ~(0x3UL << pos);
  GPIOx->OSPEEDR |= speed << pos;

  GPIOx->PUPDR &= ~(0x3UL << pos);
  GPIOx->PUPDR |= pupd << pos;
}

void gpio_afpin_init(gpio_pin_t pin, gpio_output_speed_t speed) {
  gpio_pin_init(pin, GPIO_AF, GPIO_PUSHPULL, speed, GPIO_NOPUPD);

  gpio_reg_t *GPIOx = GPIO(PINBANK(pin));
  uint32_t pos      = PINNUM(pin) & 0x7UL;
  if (PINNUM(pin) < 8) {
    GPIOx->AFLR &= ~(0xFUL << (4 * pos));
    GPIOx->AFLR |= PINAF(pin) << (4 * pos);
  } else {
    GPIOx->AFHR &= ~(0xFUL << (4 * pos));
    GPIOx->AFHR |= PINAF(pin) << (4 * pos);
  }
}
