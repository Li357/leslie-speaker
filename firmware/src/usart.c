#include "usart.h"
#include "gpio.h"
#include "rcc.h"

usart_reg_t *USART1 = ((usart_reg_t *)USART1_BASE);
usart_reg_t *USART2 = ((usart_reg_t *)USART2_BASE);
usart_reg_t *USART3 = ((usart_reg_t *)USART3_BASE);
usart_reg_t *UART4  = ((usart_reg_t *)UART4_BASE);
usart_reg_t *UART5  = ((usart_reg_t *)UART5_BASE);
usart_reg_t *USART6 = ((usart_reg_t *)USART6_BASE);
usart_reg_t *UART7  = ((usart_reg_t *)UART7_BASE);
usart_reg_t *UART8  = ((usart_reg_t *)UART8_BASE);

void usart_init(usart_reg_t *usart, uint32_t baudrate) {
  // Enable APB clock and setup GPIO pins
  RCC->APB1RSTR |= (1UL << 18);
  RCC->APB1RSTR &= ~(1UL << 18);
  RCC->APB1ENR |= USART3_CLK_BIT;
  gpio_afpin_init(USART3_TX, GPIO_VHIGHSPEED);
  gpio_afpin_init(USART3_RX, GPIO_VHIGHSPEED);
  GPIO(PINBANK(USART3_TX))->PUPDR &= ~(0b11 << (PINNUM(USART3_TX << 1)));
  GPIO(PINBANK(USART3_TX))->PUPDR |= 0b01 << (PINNUM(USART3_TX << 1));

  // uint32_t clock = usart == USART6 || usart == USART1 ? APB2_CLOCK : APB1_CLOCK;

  // Calculate division rate
  usart->CR1 = 0;
  usart->BRR = APB1_CLOCK / baudrate;
  // usart->BRR       = ((uartdiv / 16) << USART_BRR_QSHIFT | (uartdiv % 16));

  // Enable USART transmit
  usart->CR1 |= USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;
}

void usart_transmit(usart_reg_t *usart, char *ptr, int len) {
  while (!(usart->ISR & USART_ISR_TXE)) {}
  for (int i = 0; i < len; i++, ptr++) {
    usart->TDR = *ptr;
    while (!(usart->ISR & USART_ISR_TXE)) {}
  }
}
