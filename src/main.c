#include <stdint.h>

/*
 * UART4 TX on PA0 (Arduino D1) — B-L475E-IOT01A / STM32L475VG
 *
 * Docs:
 *   RM0351  — registers (RCC §6, GPIO §8, USART §40, memory map §2)
 *   DS11585 — pin AF table (PA0 → UART4_TX = AF8)
 *   UM2153  — board connectors (D1, ST-Link VCP = USART1 not UART4)
 *   See README.md for a full code ↔ manual map.
 */

/* Base Addresses — RM0351 §2 Memory map */
#define PERIPH_BASE     (0x40000000U)
#define APB1PERIPH_BASE (PERIPH_BASE)                 /* 0x40000000 */
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000U)   /* 0x40020000 */
#define AHB2PERIPH_BASE (PERIPH_BASE + 0x08000000U)   /* 0x48000000 */
#define UART4_BASE      (APB1PERIPH_BASE + 0x4C00U)   /* 0x40004C00 — RM0351 UART4 */
#define RCC_BASE        (AHB1PERIPH_BASE + 0x1000U)   /* 0x40021000 — RM0351 RCC (AHB1!) */
#define GPIOA_BASE      (AHB2PERIPH_BASE + 0U)          /* 0x48000000 */

/* RCC — RM0351 §6.4 */
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE))           /* §6.4.1 offset 0x00 */
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58U))   /* UART4EN */
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4CU))   /* GPIOAEN */

/* UART4 / USART register map — RM0351 §40.8 */
#define USART_CR1       (*(volatile uint32_t *)(UART4_BASE))         /* 0x00 */
#define USART_CR2       (*(volatile uint32_t *)(UART4_BASE + 0x4U))  /* 0x04 */
#define USART_BRR       (*(volatile uint32_t *)(UART4_BASE + 0x0CU)) /* 0x0C */
#define USART_ISR       (*(volatile uint32_t *)(UART4_BASE + 0x1CU)) /* 0x1C */
#define USART_TDR       (*(volatile uint32_t *)(UART4_BASE + 0x28U)) /* 0x28 */

/* GPIO register layout — RM0351 §8.4 */
typedef struct
{
    volatile uint32_t MODER;    // 0x00  §8.4.1
    volatile uint32_t OTYPER;   // 0x04
    volatile uint32_t OSPEEDR;  // 0x08
    volatile uint32_t PUPDR;    // 0x0C
    volatile uint32_t IDR;      // 0x10
    volatile uint32_t ODR;      // 0x14
    volatile uint32_t BSRR;     // 0x18
    volatile uint32_t LCKR;     // 0x1C
    volatile uint32_t AFRL;     // 0x20  pins 0–7
    volatile uint32_t AFRH;     // 0x24  pins 8–15
    volatile uint32_t BRR;      // 0x28
    volatile uint32_t ASCR;     // 0x2C
} GPIO_TypeDef;

/* GPIOA — RM0351 §8 + datasheet AF table */
#define GPIOA           ((GPIO_TypeDef *)(GPIOA_BASE))
#define GPIOA_CLOCK     (1U << 0)       /* RCC_AHB2ENR GPIOAEN */
#define PA0_MASK        (0xFFFFFFFCU)  /* clear MODER[1:0] */
#define UART_OUTPUT     (1U << 1)      /* MODER[1:0]=10 alternate function */

/* MSI — RM0351 RCC_CR: MSIRANGE[7:4], MSIRGSEL bit 3, MSIRDY bit 1 */
#define MSI_MASK        (0xFFFFFF0FU)
#define MSI_16MHZ       (1U << 7)      /* MSIRANGE = 1000 → ~16 MHz */
#define MSI_RANGE_SEL   (1U << 3)      /* MSIRGSEL: use MSIRANGE from CR */

/* UART4 enable + baud — RM0351 RCC_APB1ENR1 UART4EN; BRR with OVER8=0 */
#define UART4_EN        (1 << 19)      /* bit 19 */
#define BAUD_DIV_115200 (139)          /* ≈ 16 MHz / 115200 */

/* USART_CR1 — RM0351 §40.8.1 (M1 bit28, M0 bit12, TE bit3, UE bit0) */
#define WORD_LENGTH_1   (0 << 28) // M1
#define WORD_LENGTH_0   (0 << 12) // M0
#define WORD_LENGTH     (WORD_LENGTH_1 | WORD_LENGTH_0) /* 00 = 8 data bits */
#define TX_ENABLE       (1 << 3)  /* TE */
#define UART_ENABLE     (1 << 0)  /* UE */

/* USART_CR2 — RM0351 §40.8.2 STOP[13:12]: 10 = 2 stop bits */
#define STOP_1          (1 << 13)
#define STOP_2          (0 << 12)
#define STOP_BITS       (STOP_1 | STOP_2)

/* USART_ISR — RM0351 TXE bit 7 */
#define UART_TXE        (0x80U)

/* Datasheet AF table: PA0 UART4_TX = AF8 → AFRL[3:0] = 8 */
#define AF8_UART4       (1 << 3)

void UART_Setup(void)
{
    /* 1) Peripheral clock — RM0351 RCC_APB1ENR1.UART4EN */
    RCC_APB1ENR1 |= UART4_EN;

    /* 2) Frame — CR1/CR2 while UE=0 — RM0351 §40.5 / §40.8 */
    USART_CR1 |= WORD_LENGTH; /* 8 data bits */
    USART_CR1 |= TX_ENABLE;   /* TE */
    USART_CR2 |= STOP_BITS;   /* 2 stop bits */

    /* 3) MSI 16 MHz — RM0351 RCC_CR MSIRGSEL + MSIRANGE, wait MSIRDY */
    RCC_CR &= MSI_MASK;
    RCC_CR |= MSI_RANGE_SEL | MSI_16MHZ;
    while (!(RCC_CR & (1U << 1)))
    {
    }

    /* 4) Baud — RM0351 USART_BRR, OVER8=0: BRR ≈ fck/baud */
    USART_BRR = BAUD_DIV_115200;

    /* 5) Enable USART last — RM0351 UE */
    USART_CR1 |= UART_ENABLE;
}

void UART4_GPIO_Init(void)
{
    /* RM0351: enable GPIOA, MODER=AF, AFRL=AF8 (datasheet PA0/UART4_TX) */
    RCC_AHB2ENR |= GPIOA_CLOCK;
    GPIOA->MODER &= PA0_MASK;
    GPIOA->MODER |= UART_OUTPUT;
    GPIOA->AFRL = AF8_UART4;
}

void UART_Transmit(void)
{
    /* RM0351 USART_ISR.TXE then USART_TDR */
    while (!(USART_ISR & UART_TXE))
        ;
    USART_TDR = 'A';
}

int main(void)
{
    UART4_GPIO_Init(); /* pin mux before using UART */
    UART_Setup();

    while (1)
    {
        UART_Transmit(); /* D1 / PA0 */
    }
    return 0;
}
