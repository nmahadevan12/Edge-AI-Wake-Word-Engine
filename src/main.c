/* Unsigned Integer Types */
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

/* Signed Integer Types */
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

/* Base Addresses */
#define PERIPH_BASE     (0x40000000U)
#define APB1PERIPH_BASE (PERIPH_BASE)
#define APB2PERIPH_BASE (PERIPH_BASE + 0x10000U)
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x20000U)
#define AHB2PERIPH_BASE (PERIPH_BASE + 0x8000000U)
#define DFSDM1_BASE     (APB2PERIPH_BASE + 0x6000U)
#define UART4_BASE      (APB1PERIPH_BASE + 0x4C00U)
#define RCC_BASE        (AHB1PERIPH_BASE + 0x1000U)
#define GPIOA_BASE      (AHB2PERIPH_BASE + 0U)
#define GPIOE_BASE      (AHB2PERIPH_BASE + 0x1000U)
#define SYSTICK_BASE    (0xE000E010U)

/* RCC Register Addresses */
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58U))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x60U))
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4CU))

/* UART Register Addresses*/
#define USART_CR1       (*(volatile uint32_t *)(UART4_BASE))
#define USART_CR2       (*(volatile uint32_t *)(UART4_BASE + 0x4U))
#define USART_BRR       (*(volatile uint32_t *)(UART4_BASE + 0xCU))
#define USART_ISR       (*(volatile uint32_t *)(UART4_BASE + 0x1CU))
#define USART_TDR       (*(volatile uint32_t *)(UART4_BASE + 0x28U))

/* DFSDM Register Addresses */
#define CH0CFGR1        (*(volatile uint32_t *)(DFSDM1_BASE))
#define CH2CFGR1        (*(volatile uint32_t *)(DFSDM1_BASE + 0x40U))
#define FLT0CR1         (*(volatile uint32_t *)(DFSDM1_BASE + 0x100U))

/* SysTick Timer Addresses */
#define STK_CTRL        (*(volatile uint32_t *)(SYSTICK_BASE))
#define STK_LOAD        (*(volatile uint32_t *)(SYSTICK_BASE + 0x4U))
#define STK_VAL         (*(volatile uint32_t *)(SYSTICK_BASE + 0x8U))
#define STK_CALIB       (*(volatile uint32_t *)(SYSTICK_BASE + 0xCU))

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
#define GPIOA_CLOCK     (1 << 0)
#define PA0_MASK        (0xFFFFFFFCU)
#define UART_OUTPUT     (1 << 1)

/* GPIOE */
#define GPIOE           ((GPIO_TypeDef *)(GPIOE_BASE))
#define GPIOE_CLOCK     (1 << 4)
#define PE9_MASK        (0xFFF3FFFFU)
#define MIC_CLOCK       (1 << 19)
#define PE7_MASK        (0xFFFF3FFFU)
#define MIC_DATA        (1 << 15)

/* MSI FREQUENCY */
#define MSI_MASK        (0xFFFFFF0FU)
#define MSI_16MHZ       (1 << 7)
#define CLK_RANGE_SEL   (1 << 3)
#define MSI_CLK_SEL     (MSI_16MHZ | CLK_RANGE_SEL)

/* UART */
#define UART4_EN        (1 << 19)
#define BAUD_DIV_115200 (139) // Clock Hz (16 MHz), 115200
#define WORD_LENGTH_1   (0 << 28) // M1
#define WORD_LENGTH_0   (0 << 12) // M0
#define WORD_LENGTH     (WORD_LENGTH_1 | WORD_LENGTH_0)
#define TX_ENABLE       (1 << 3)
#define WORD_LEN_TX_EN  (WORD_LENGTH | TX_ENABLE)
#define UART_ENABLE     (1 << 0)
#define STOP_1          (1 << 13)
#define STOP_2          (0 << 12)
#define STOP_BITS       (STOP_1 | STOP_2)
#define UART_TXE        (0x80U)

/* Alternate Function Register Low */
#define AF8_UART4       (1 << 3)
#define AF6_MIC_BIT_30  (1 << 30)
#define AF6_MIC_BIT_29  (1 << 29)
#define AF6_MIC_PE7     (AF6_MIC_BIT_30 | AF6_MIC_BIT_29)

/* Alternate Function Register High */
#define AF6_MIC_BIT_6   (1 << 6)
#define AF6_MIC_BIT_5   (1 << 5)
#define AF6_MIC_PE9     (AF6_MIC_BIT_6 | AF6_MIC_BIT_5)

/* SysTick Control and Status Register */
#define AHB_CLOCK       (1 << 2)
#define COUNTER_ENABLE  (1)
#define TICK_TIME       (0x3E7FU) // 15999
#define TICK_INTERRUPT  (1 << 1)
#define CLK_SOURCE_INT  (AHB_CLOCK | TICK_INTERRUPT)

/* MIC */
#define DFSDM1EN        (1 << 24) // DFSDM1 clock enabled
#define CKOUTDIV        (0x7U) // divides output clock
#define DFSDMEN         (1 << 31) // DFSDM globally enabled
#define CHEN            (1 << 7) // channel enable
#define DFEN            (1 << 0) // digital filter enable

uint32_t timer = 0; // defined globally since it's used for interrupts

void UART_Setup(void)
{
    RCC_APB1ENR1 |= UART4_EN; // enable clock

    USART_CR1 |= WORD_LEN_TX_EN; // 1 start bit, 8 data bits; enables TX
    USART_CR2 |= STOP_BITS; // 2 stop bits

    RCC_CR &= MSI_MASK; // clear MSI bits
    RCC_CR |= MSI_CLK_SEL; // set MSI to 16 MHz, sets MSI = 1000;  use MSI range provided in RCC_CR (16 MHz)

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

void UART_Transmit(char buff)
{
    while (!(USART_ISR & UART_TXE)); // waits until data is transferred to shift register, waits until TXE bit = 1
    USART_TDR = buff; // sets Transmit Data Register = buff
}

void UART_Transmit_Ptr(char *buff)
{
    uint8_t i;
    for (i = 0; i < 4; i++) // runs four times
    {
        UART_Transmit(*buff);
        buff++;
    }
}

void Mic_Setup(void)
{
    RCC_AHB2ENR |= GPIOE_CLOCK; // enables GPIOE clock

    /* PE9: Clock */
    GPIOE->MODER &= PE9_MASK; // clear bits [19:18]
    GPIOE->MODER |= MIC_CLOCK; // alternate function mode
    GPIOE->AFRH |= AF6_MIC_PE9; // sets alternate function for PE9

    /* PE7: Data */
    GPIOE->MODER &= PE7_MASK; // clear bits [15:14]
    GPIOE->MODER |= MIC_DATA; // alternate function mode
    GPIOE->AFRL |= AF6_MIC_PE7; // sets alternate function for PE9
}

void DFSDM_Setup(void)
{
    RCC_APB2ENR |= DFSDM1EN; // DFSDM1 clock enable
    
    /*
        VAL + 1 = CLK / OUTPUT_CLK

        CLK: 16 MHz, OUTPUT_CLK: 2 MHz
        VAL = 8 - 1 = 7
    */
    CH0CFGR1 |= CKOUTDIV; // divides output clock

    /* Clock divider, then enable filter */
    CH0CFGR1 |= DFSDMEN; // enable DFSDM interface

    CH2CFGR1 |= CHEN; // channel 2 enable, PE7

    FLT0CR1 |= DFEN; // digital filter 0 enable
}

void Convert_To_Bytes(uint32_t value) // uint32_t, big endian
{
    unsigned char buffer[4]; // buffer = 4 bytes

    buffer[0] = (unsigned char) ((value & 0xFF000000UL) >> 24); // LSR 3 bytes
    buffer[1] = (unsigned char) ((value & 0x00FF0000UL) >> 16); // LSR 2 bytes
    buffer[2] = (unsigned char) ((value & 0x0000FF00UL) >> 8);  // LSR 1 byte
    buffer[3] = (unsigned char) (value  & 0x000000FFUL);
    UART_Transmit_Ptr(buffer);
}

void SysTick_Init(void)
{
    STK_CTRL |= CLK_SOURCE_INT; // sets clock source to 16 MHz, counting down to 0 enables interrupt 

    /*  
        MSI (16 MHz)
        SYSCLK (default = MSI)
        divide HPRE (default = 1)
        HCLK (16 MHz) 
        TICK_TIME = (HCLK / 1000) - 1

        1000 = CLOCKS/SEC -> 1 CLK every 1 ms
    */
    STK_LOAD |= TICK_TIME; // 1 ms increment (time to count down from 15999 to 0)
    STK_CTRL |= COUNTER_ENABLE; // enable SysTick counter
}

/*
    PE9, MEMS microphone, DFSDM1_CKOUT
*/

void SysTick_Handler(void)
{
    /* Executed on each 1ms interrupt */
    timer++; // increment timer
}

int main(void)
{
    UART4_GPIO_Init(); // initialize UART4 pin
    UART_Setup(); // setup UART
    SysTick_Init(); // initialize SysTick
    Mic_Setup(); // setup mic
    DFSDM_Setup(); // digital filter setup

    uint32_t number = 0;
    volatile uint32_t x; // used for delay

    while (1)
    {
        Convert_To_Bytes(number);
        Convert_To_Bytes(timer);

        number++; // increment number
        for (x = 0; x < (1000000); x++); // delay
    }
    return 0; // never reached
}