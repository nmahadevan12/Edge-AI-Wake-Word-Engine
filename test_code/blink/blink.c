#include <stdint.h>

/* Base Addresses */
#define PERIPH_BASE     0x40000000U // + for address, | for mask
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x20000U)
#define AHB2PERIPH_BASE (PERIPH_BASE + 0x08000000U)
#define GPIOA_BASE      (AHB2PERIPH_BASE + 0U)


/* RCC Register Addresses */
#define RCC             (AHB1PERIPH_BASE + 0x1000U)
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC + 0x4CU)) // treat number as the address of a variable
    /*
    RCC + 0x4CU: math value, integer
    volatile uint32_t *: converts from math value to an address
    *: tells to output the math value inside the address, lvalue
    */

/* GPIOA Peripheral Register Struct */
typedef struct
{
    volatile uint32_t MODER;    // 0x00
    volatile uint32_t OTYPER;   // 0x04
    volatile uint32_t OSPEEDR;  // 0x08
    volatile uint32_t PUPDR;    // 0x0C
    volatile uint32_t IDR;      // 0x10
    volatile uint32_t ODR;      // 0x14
} GPIO_TypeDef;
#define GPIOA ((GPIO_TypeDef *)(GPIOA_BASE))

#define GPIOA_CLOCK (1U << 0)
#define LED_OUTPUT  (1U << 10)
#define LED_PIN     (1U << 5)

volatile uint16_t counter;

void ledSetup(void)
{
    RCC_AHB2ENR |= GPIOA_CLOCK; // enables GPIOA clock
    (void)RCC_AHB2ENR;          // wait for GPIOA clock before touching registers

    GPIOA->MODER &= ~(3U << 10); // clear MODER bits [11:10]
    GPIOA->MODER |= LED_OUTPUT;  // set PIN A5 to output
    GPIOA->ODR |= LED_PIN;      // sets PIN A5 to LOW (use |= LED_PIN for HIGH)
}

int main(void)
{
    ledSetup();

    while (1)
    {
        for (counter = 0; counter < 10000; counter ++); // delay
        GPIOA->ODR ^= LED_PIN; 
    }

    return 0;
}
