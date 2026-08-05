/* Unsigned Integer Types */
typedef unsigned    char        uint8_t
typedef unsigned    short       uint16_t
typedef unsigned    long        uint32_t
typedef unsigned    long long   uint64_t

/* Signed Integer Types */
typedef signed      char        int8_t
typedef signed      short       int16_t
typedef signed      long        int32_t
typedef signed      long long   int64_t

/* Base Addresses */
#define PERIPH_BASE     (0x40000000U)
#define APB1PERIPH_BASE (PERIPH_BASE)
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000U)
#define AHB2PERIPH_BASE (PERIPH_BASE + 0x08000000U)
#define UART4_BASE      (APB1PERIPH_BASE + 0x4C00U)
#define RCC_BASE        (AHB1PERIPH_BASE + 0x1000U)
#define GPIOA_BASE      (AHB2PERIPH_BASE + 0U)

/* RCC Register Addresses */
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58U))
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4CU))

/* UART Register Addresses*/
#define USART_CR1       (*(volatile uint32_t *)(UART4_BASE))
#define USART_CR2       (*(volatile uint32_t *)(UART4_BASE + 0x4U))
#define USART_BRR       (*(volatile uint32_t *)(UART4_BASE + 0x0CU))
#define USART_ISR       (*(volatile uint32_t *)(UART4_BASE + 0x1CU))
#define USART_TDR       (*(volatile uint32_t *)(UART4_BASE + 0x28U))

/* GPIO Peripheral Register Struct */
typedef struct
{
    volatile uint32_t MODER;    // 0x00
    volatile uint32_t OTYPER;   // 0x04
    volatile uint32_t OSPEEDR;  // 0x08
    volatile uint32_t PUPDR;    // 0x0C
    volatile uint32_t IDR;      // 0x10
    volatile uint32_t ODR;      // 0x14
    volatile uint32_t BSRR;     // 0x18
    volatile uint32_t LCKR;     // 0x1C
    volatile uint32_t AFRL;     // 0x20
    volatile uint32_t AFRH;     // 0x24
    volatile uint32_t BRR;      // 0x28
    volatile uint32_t ASCR;     // 0x2C
} GPIO_TypeDef;

/* GPIOA */
#define GPIOA           ((GPIO_TypeDef *)(GPIOA_BASE))
#define GPIOA_CLOCK     (1U << 0)
#define PA0_MASK        (0xFFFFFFFCU)
#define UART_OUTPUT     (1U << 1)

/* MSI FREQUENCY (RCC_CR MSIRANGE[7:4], need MSIRGSEL=1) */
#define MSI_MASK        (0xFFFFFF0FU)
#define MSI_16MHZ       (1U << 7)   /* MSIRANGE = 1000 → ~16 MHz */
#define MSI_RANGE_SEL   (1U << 3)   /* use MSIRANGE from RCC_CR */

/* UART */
#define UART4_EN        (1 << 19)
#define BAUD_DIV_115200 (139) // Clock Hz (16 MHz), 115200

/* UART Control Register 1 */
#define WORD_LENGTH_1   (0 << 28) // M1
#define WORD_LENGTH_0   (0 << 12) // M0
#define WORD_LENGTH     (WORD_LENGTH_1 | WORD_LENGTH_0)
#define TX_ENABLE       (1 << 3)
#define UART_ENABLE     (1 << 0)

/* UART Control Register 2 */
#define STOP_1          (1 << 13)
#define STOP_2          (0 << 12)
#define STOP_BITS       (STOP_1 | STOP_2)

/* UART Interrupt and Status Register */
#define UART_TXE        (0x80U)

/* Alternate Function Register Low */
#define AF8_UART4       (1 << 3)

void UART_Setup(void)
{
    RCC_APB1ENR1 |= UART4_EN; // enable clock

    USART_CR1 |= WORD_LENGTH; // 1 start bit, 8 data bits
    USART_CR1 |= TX_ENABLE; // enables TX
    USART_CR2 |= STOP_BITS; // 2 stop bits

    RCC_CR &= MSI_MASK; // clear MSI bits
    RCC_CR |= MSI_RANGE_SEL | MSI_16MHZ; // use CR range, MSI ~16 MHz
    while (!(RCC_CR & (1U << 1))) // wait MSIRDY
    {
    }
    USART_BRR = BAUD_DIV_115200;

    USART_CR1 |= UART_ENABLE;
}

void UART4_GPIO_Init(void)
{
    RCC_AHB2ENR |= GPIOA_CLOCK; // enables GPIOA clock
    GPIOA->MODER &= PA0_MASK; // clears bits [1:0]
    GPIOA->MODER |= UART_OUTPUT; // alternate fucntion mode (UART)
    GPIOA->AFRL = AF8_UART4; // sets alternate function for PA0
}

void UART_Transmit(void)
{
    while (!(USART_ISR & UART_TXE)); // waits until data is transferred to shift register, waits until TXE bit = 1
    USART_TDR = 'A'; // send char A
}

int main(void)
{
    UART4_GPIO_Init(); // initialize UART4 pin
    UART_Setup(); // setup UART

    while (1)
    {
        UART_Transmit(); // transmit UART on D1, PA0
    }
    return 0; // never reached
}