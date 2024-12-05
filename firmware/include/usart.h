#pragma once

#include "config.h"
#include "gpio.h"
#include "stm32f7.h"

#define USART_ISR_TC     (1UL << 6)
#define USART_ISR_TXE    (1UL << 7)
#define USART_BRR_QSHIFT (4UL)
#define USART_CR1_TE     (1UL << 3)
#define USART_CR1_RE     (1UL << 2)
#define USART_CR1_UE     (1UL << 0)

#define USART1_CLK_BIT   (1UL << 4)
#define USART2_CLK_BIT   (1UL << 17)
#define USART3_CLK_BIT   (1UL << 18)
#define UART4_CLK_BIT    (1UL << 19)
#define UART5_CLK_BIT    (1UL << 20)
#define USART6_CLK_BIT   (1UL << 5)
#define UART7_CLK_BIT    (1UL << 30)
#define UART8_CLK_BIT    (1UL << 31)

#define USART3_TX        AFPIN('D', 8, 7)
#define USART3_RX        AFPIN('D', 9, 7)

#define UART_INIT(usart, apb)                     \
  {                                               \
    apb |= usart##_CLK_BIT;                       \
    gpio_afpin_init(usart##_TX, GPIO_VHIGHSPEED); \
    gpio_afpin_init(usart##_RX, GPIO_VHIGHSPEED); \
  }

#define USART_INIT(usart, apb)                    \
  {                                               \
    UART_INIT(usart, apb);                        \
    gpio_afpin_init(usart##_CK, GPIO_VHIGHSPEED); \
  }

typedef volatile struct {
  uint32_t CR1;
  uint32_t CR2;
  uint32_t CR3;
  uint32_t BRR;
  uint32_t GTPR;
  uint32_t RTOR;
  uint32_t RQR;
  uint32_t ISR;
  uint32_t ICR;
  uint32_t RDR;
  uint32_t TDR;
} usart_reg_t;

extern usart_reg_t *USART3;

void usart_init(usart_reg_t *usart, uint32_t baudrate);
void usart_transmit(usart_reg_t *usart, char *ptr, int len);
