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
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x20000U)
#define AHB2PERIPH_BASE (PERIPH_BASE + 0x8000000U)
#define UART4_BASE      (APB1PERIPH_BASE + 0x4C00U)
#define RCC_BASE        (AHB1PERIPH_BASE + 0x1000U)
#define GPIOA_BASE      (AHB2PERIPH_BASE + 0U)
#define GPIOC_BASE      (AHB2PERIPH_BASE + 0x800U)
#define ADC_BASE        (AHB2PERIPH_BASE + 0x8040000U)
#define ADC1_BASE       (ADC_BASE)
#define SYSTICK_BASE    (0xE000E010U)

/* RCC Register Addresses */
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58U))
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4CU))
#define RCC_CCIPR       (*(volatile uint32_t *)(RCC_BASE + 0x88U))

/* UART Register Addresses*/
#define USART_CR1       (*(volatile uint32_t *)(UART4_BASE))
#define USART_CR2       (*(volatile uint32_t *)(UART4_BASE + 0x4U))
#define USART_BRR       (*(volatile uint32_t *)(UART4_BASE + 0xCU))
#define USART_ISR       (*(volatile uint32_t *)(UART4_BASE + 0x1CU))
#define USART_TDR       (*(volatile uint32_t *)(UART4_BASE + 0x28U))

/* ADC Addresses */
#define ADC_ISR         (*(volatile uint32_t *)(ADC1_BASE))
#define ADC_CR          (*(volatile uint32_t *)(ADC1_BASE + 0x8U))
#define ADC_CFGR        (*(volatile uint32_t *)(ADC1_BASE + 0xCU))
#define ADC_SMPR2       (*(volatile uint32_t *)(ADC1_BASE + 0x18U))
#define ADC_SQR1        (*(volatile uint32_t *)(ADC1_BASE + 0x30U))
#define ADC_DR          (*(volatile uint32_t *)(ADC1_BASE + 0x40U))
#define ADC_CCR         (*(volatile uint32_t *)(ADC1_BASE + 0x308U))

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

/* GPIOC */
#define GPIOC           ((GPIO_TypeDef *)(GPIOC_BASE))
#define GPIOC_CLOCK     (1 << 2)
#define PC5_MASK        (0xFFFFF3FFU)
#define ANALOG_BIT_11   (1 << 11)
#define ANALOG_BIT_10   (1 << 10)
#define ANALOG_FUNC_5   (ANALOG_BIT_11 | ANALOG_BIT_10) // 11
#define PC4_MASK        (0xFFFFFCFFU)
#define ANALOG_BIT_9    (1 << 9)
#define ANALOG_BIT_8    (1 << 8)
#define ANALOG_FUNC_4   (ANALOG_BIT_9 | ANALOG_BIT_8) // 11

/* MSI FREQUENCY */
#define MSI_MASK        (0xFFFFFF0FU)
#define MSI_16MHZ       (1 << 7)
#define CLK_RANGE_SEL   (1 << 3)
#define MSI_FREQ_SEL    (MSI_16MHZ | CLK_RANGE_SEL)

/* UART */
#define WORD_LENGTH_1   (0 << 28) // M1
#define WORD_LENGTH_0   (0 << 12) // M0
#define WORD_LENGTH     (WORD_LENGTH_1 | WORD_LENGTH_0)
#define TX_ENABLE       (1 << 3)
#define UART_ENABLE     (1 << 0)
#define LENGTH_TX       (WORD_LENGTH | TX_ENABLE)
#define STOP_1          (1 << 13)
#define STOP_2          (0 << 12)
#define STOP_BITS       (STOP_1 | STOP_2)
#define UART_TXE        (0x80U)
#define UART4_EN        (1 << 19)
#define BAUD_DIV_115200 (139) // Clock Hz (16 MHz), 115200
#define AF8_UART4       (1 << 3) // Alternate Function for UART

/* ADC */
#define RESOLUTION      (1 << 4) // bits [4:3] = 10
#define ADC_PRESCALER   (0x3F2) // 0b1010
#define CLOCK_MODE      (1 << 16)
#define PRESCALER_CLOCK (ADC_PRESCALER | CLOCK_MODE)
#define CLOCK_BIT_29    (1 << 29)
#define CLOCK_BIT_28    (1 << 28)
#define ADC_CLOCK       (CLOCK_BIT_29 | CLOCK_BIT_28) // 11
#define ADCEN           (1 << 13)
#define ADVREGEN        (1 << 28) // ADC Voltage Regulator Enable
#define CHN_PC5         (14) // A0
#define CHN_PC4         (13) // A1
#define CONVERSION_NUM  (0x1U) // number of conversions = 2
#define DEEPPWD         (1 << 29) // deep-power-down enable bit for ADC
#define ADCAL           (1 << 31) // ADC calibration
#define ADEN            (1 << 0) // ADC enable control
#define ADRDY           (1 << 0) // ADC ready bit
#define SAMPLING_CHN_14 (1 << 14) // 100 = 47.5 ADC clock cycles
#define SAMPLING_CHN_13 (1 << 11) // 100 = 47.5 ADC clock cycles
#define SAMPLING_CHN    (SAMPLING_CHN_14 | SAMPLING_CHN_13)
#define ADSTART         (1 << 2)

/* SysTick Control and Status Register */
#define AHB_CLOCK       (1 << 2)
#define COUNTER_ENABLE  (1)
#define TICK_TIME       (0x3E7FU) // 15999
#define TICK_INTERRUPT  (1 << 1)
#define CLK_SOURCE_INT  (AHB_CLOCK | TICK_INTERRUPT)

char buffer[4]; // buffer = 4 bytes
uint32_t timer = 0; // defined globally since it's used for interrupts

void UART_Setup(void)
{
    RCC_APB1ENR1 |= UART4_EN; // enable clock
    USART_CR1 |= LENGTH_TX; // sets length (1 start bit, 8 data bits), enables tx
    USART_CR2 |= STOP_BITS; // 2 stop bits
    RCC_CR &= MSI_MASK; // clear MSI bits
    RCC_CR |= MSI_FREQ_SEL; // set MSI to 16 MHz (MSI = 1000), use MSI range provided in RCC_CR (16 MHz)
    USART_BRR = BAUD_DIV_115200;
    USART_CR1 |= UART_ENABLE;
}

void UART4_GPIO_Init(void)
{
    RCC_AHB2ENR |= GPIOA_CLOCK; // enables GPIOA clock
    /*
        MODER
        00: Input
        01: Output
        10: Alternate Function
        11: Analog
    */
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

/* 
    A0, PC5, ADC 
    A1, PC4, ADC
*/

void ADC_Pin_Setup(void)
{
    RCC_AHB2ENR |= GPIOC_CLOCK; // enables GPIOC clock

    GPIOC->MODER &= PC5_MASK; // clears bits [11:10]
    GPIOC->MODER |= ANALOG_FUNC_5; // analog mode for PC5

    GPIOC->MODER &= PC4_MASK; // clears bits [9:8]
    GPIOC->MODER |= ANALOG_FUNC_4; // analog mode for PC4
}

void ADC_Clock_Init(void)
{
    RCC_CCIPR |= ADC_CLOCK; // selects system clock as ADC clock
    ADC_CCR |= PRESCALER_CLOCK; // divides clock by 128, sets clock = HCLK
    RCC_AHB2ENR |= ADCEN; // enabled ADC clock
}

void ADC_Init(void)
{
    ADC_CFGR |= RESOLUTION; // 8-bit resolution

    ADC_CR &= ~DEEPPWD; // disables deep-power-down mode for ADC
    ADC_CR |= ADVREGEN; // turns on ADC's internal power supply

    volatile int delay;
    for (delay = 0; delay < 360; delay++); // ~20uS delay

    ADC_CR |= ADCAL; // enables ADC calibration
    while (ADC_CR & ADCAL); // waits for ADCAL to be 0 (calibration complete)

    ADC_CR |= ADEN; // ADC enable
    while (!(ADC_ISR & ADRDY)); // waits for ADC to be ready

    /*
        enable ADC sampling for Channel 14
        enable ADC sampling for Channel 13
    */
    ADC_SMPR2 |= SAMPLING_CHN;
}

uint8_t Get_ADC_Val(unsigned int channel)
{
    ADC_SQR1 = channel;

    ADC_CR |= ADSTART; // 1 to start converison
    while (ADC_CR & ADSTART); // 0 if no converison is ongoing
    return (ADC_DR & 0xFFU);
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

void SysTick_Handler(void)
{
    /* Executed on each 1ms interrupt */
    timer++; // increment timer
}

void ADC_Int_To_Bytes(uint32_t ADC_Int) // uint32_t, big endian
{
    buffer[0] = ((ADC_Int & 0xFF000000U) >> 24); // LSR 3 bytes
    buffer[1] = ((ADC_Int & 0xFF0000U) >> 16); // LSR 2 bytes
    buffer[2] = ((ADC_Int & 0xFF00U) >> 8); // LSR 1 byte
    buffer[3] = (ADC_Int & 0xFFU);
}

void Timer_To_Bytes(void) // uint32_t, big endian
{
    buffer[0] = ((timer & 0xFF000000U) >> 24); // LSR 3 bytes
    buffer[1] = ((timer & 0xFF0000U) >> 16); // LSR 2 bytes
    buffer[2] = ((timer & 0xFF00U) >> 8); // LSR 1 byte
    buffer[3] = (timer & 0xFFU);
}

int main(void)
{
    UART4_GPIO_Init(); // initialize UART4 pin
    UART_Setup(); // setup UART
    ADC_Pin_Setup(); // sets up A0 and A1 as ADC pins
    ADC_Clock_Init(); // initialized ADC's clock
    ADC_Init(); // initializes ADC
    SysTick_Init(); // initialize SysTick

    uint8_t ADC_Val;
    uint16_t ADC_Buffer = 0;

    while (1)
    {
        ADC_Val = Get_ADC_Val(CHN_PC5 << 6);
        ADC_Buffer = ADC_Val;

        ADC_Val = Get_ADC_Val(CHN_PC4 << 6);
        ADC_Buffer |= (ADC_Val << 8);

        ADC_Int_To_Bytes(ADC_Buffer);
        UART_Transmit_Ptr(buffer); // transmit ADC_Int on D1, PA0

        Timer_To_Bytes();
        UART_Transmit_Ptr(buffer); // transmit Timer on D1, PA0
    }
    return 0; // never reached
}