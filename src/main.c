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
#define I2C2_BASE       (APB1PERIPH_BASE + 0x5800U)
#define RCC_BASE        (AHB1PERIPH_BASE + 0x1000U)
#define GPIOA_BASE      (AHB2PERIPH_BASE + 0U)
#define GPIOB_BASE      (AHB2PERIPH_BASE + 0x400U)
#define SYSTICK_BASE    (0xE000E010U)

/* RCC Register Addresses */
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58U))
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4CU))

/* UART Register Addresses */
#define USART_CR1       (*(volatile uint32_t *)(UART4_BASE))
#define USART_CR2       (*(volatile uint32_t *)(UART4_BASE + 0x4U))
#define USART_BRR       (*(volatile uint32_t *)(UART4_BASE + 0xCU))
#define USART_ISR       (*(volatile uint32_t *)(UART4_BASE + 0x1CU))
#define USART_TDR       (*(volatile uint32_t *)(UART4_BASE + 0x28U))

/* I2C2 Register Addresses */
#define I2C2_CR1        (*(volatile uint32_t *)(I2C2_BASE + 0x00U))
#define I2C2_CR2        (*(volatile uint32_t *)(I2C2_BASE + 0x04U))
#define I2C2_TIMINGR    (*(volatile uint32_t *)(I2C2_BASE + 0x10U))
#define I2C2_ISR        (*(volatile uint32_t *)(I2C2_BASE + 0x18U))
#define I2C2_ICR        (*(volatile uint32_t *)(I2C2_BASE + 0x1CU))
#define I2C2_RXDR       (*(volatile uint32_t *)(I2C2_BASE + 0x24U))
#define I2C2_TXDR       (*(volatile uint32_t *)(I2C2_BASE + 0x28U))

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

/* GPIOA / GPIOB */
#define GPIOA           ((GPIO_TypeDef *)(GPIOA_BASE))
#define GPIOB           ((GPIO_TypeDef *)(GPIOB_BASE))
#define GPIOA_CLOCK     (1U << 0)
#define GPIOB_CLOCK     (1U << 1)
#define PA0_MASK        (0xFFFFFFFCU)
#define UART_OUTPUT     (1U << 1)

/* MSI FREQUENCY */
#define MSI_MASK        (0xFFFFFF0FU)
#define MSI_16MHZ       (1 << 7)
#define CLK_RANGE_SEL   (1 << 3)
#define MSI_CLK_SEL     (MSI_16MHZ | CLK_RANGE_SEL)

/* UART */
#define UART4_EN        (1 << 19)
#define BAUD_DIV_115200 (139) // Clock Hz (16 MHz), 115200

/* UART Control Register 1 */
#define WORD_LENGTH_1   (0 << 28) // M1
#define WORD_LENGTH_0   (0 << 12) // M0
#define WORD_LENGTH     (WORD_LENGTH_1 | WORD_LENGTH_0)
#define TX_ENABLE       (1 << 3)
#define WORD_LEN_TX_EN  (WORD_LENGTH | TX_ENABLE)
#define UART_ENABLE     (1 << 0)

/* UART Control Register 2 */
#define STOP_1          (1 << 13)
#define STOP_2          (0 << 12)
#define STOP_BITS       (STOP_1 | STOP_2)

/* UART Interrupt and Status Register */
#define UART_TXE        (0x80U)

/* Alternate Function Register Low */
#define AF8_UART4       (1 << 3)

/* SysTick Control and Status Register */
#define AHB_CLOCK       (1 << 2)
#define COUNTER_ENABLE  (1)
#define TICK_TIME       (0x3E7FU) // 15999
#define TICK_INTERRUPT  (1 << 1)
#define CLK_SOURCE_INT  (AHB_CLOCK | TICK_INTERRUPT)

/* I2C2 */
#define I2C2_EN         (1U << 22)
#define I2C_PE          (1U << 0)
#define I2C_START       (1U << 13)
#define I2C_STOP        (1U << 14)
#define I2C_RD_WRN      (1U << 10)
#define I2C_AUTOEND     (1U << 25)
#define I2C_TXIS        (1U << 1)
#define I2C_RXNE        (1U << 2)
#define I2C_NACKF       (1U << 4)
#define I2C_STOPF       (1U << 5)
#define I2C_TC          (1U << 6)
#define I2C_NACKCF      (1U << 4)
#define I2C_STOPCF      (1U << 5)
/* ~100 kHz @ 16 MHz I2CCLK (RM0351 TIMINGR) */
#define I2C2_TIMING_100K (0x00303D5BU)
#define I2C_TIMEOUT     (100000U)

/* LSM6DSL on B-L475E-IOT01A (I2C2, PB10/PB11) */
#define LSM6DSL_ADDR    (0x6AU) // 7-bit
#define LSM6DSL_WHO_AM_I_REG (0x0FU)
#define LSM6DSL_WHO_AM_I_VAL (0x6AU)
#define LSM6DSL_CTRL1_XL (0x10U)
#define LSM6DSL_CTRL2_G  (0x11U)
#define LSM6DSL_CTRL3_C  (0x12U)
#define LSM6DSL_OUTX_L_G (0x22U)
/* CTRL1_XL: ODR 104 Hz, ±2g */
#define LSM6DSL_CTRL1_XL_VAL (0x40U)
/* CTRL2_G: ODR 104 Hz, +/-250 dps */
#define LSM6DSL_CTRL2_G_VAL  (0x40U)
/* CTRL3_C: IF_INC auto-increment */
#define LSM6DSL_CTRL3_C_VAL  (0x04U)

/* Gesture pipeline */
#define SAMPLE_PERIOD_MS (20U)   // 50 Hz
#define BUF_LEN          (64U)
#define SHAKE_ENERGY_THR (50000000LL) // mean-removed energy
#define TILT_MEAN_THR    (8000)       // ~0.5 g in +/-2g LSB

uint32_t timer = 0; // defined globally since it's used for interrupts
volatile uint32_t sample_flag = 0;

int16_t ax_buf[BUF_LEN];
int16_t ay_buf[BUF_LEN];
int16_t az_buf[BUF_LEN];
uint32_t buf_idx = 0;
uint32_t buf_filled = 0;

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

void SysTick_Handler(void)
{
    /* Executed on each 1ms interrupt */
    timer++; // increment timer
    if ((timer % SAMPLE_PERIOD_MS) == 0U)
    {
        sample_flag = 1U;
    }
}

static int I2C2_Wait_Flag(uint32_t flag, uint32_t set)
{
    uint32_t t = I2C_TIMEOUT;
    while (t--)
    {
        if (set)
        {
            if (I2C2_ISR & flag)
            {
                return 0;
            }
        }
        else
        {
            if (!(I2C2_ISR & flag))
            {
                return 0;
            }
        }
        if (I2C2_ISR & I2C_NACKF)
        {
            I2C2_ICR = I2C_NACKCF | I2C_STOPCF;
            I2C2_CR2 |= I2C_STOP;
            return -1;
        }
    }
    return -1;
}

void I2C2_GPIO_Init(void)
{
    /* PB10 = I2C2_SCL, PB11 = I2C2_SDA, AF4, open-drain, pull-up */
    RCC_AHB2ENR |= GPIOB_CLOCK;

    /* MODER: alternate function (10) for PB10 and PB11 */
    GPIOB->MODER &= ~((3U << 20) | (3U << 22));
    GPIOB->MODER |=  ((2U << 20) | (2U << 22));

    /* OTYPER: open-drain */
    GPIOB->OTYPER |= (1U << 10) | (1U << 11);

    /* OSPEEDR: high speed */
    GPIOB->OSPEEDR |= (3U << 20) | (3U << 22);

    /* PUPDR: pull-up */
    GPIOB->PUPDR &= ~((3U << 20) | (3U << 22));
    GPIOB->PUPDR |=  ((1U << 20) | (1U << 22));

    /* AFRH: AF4 for pin 10 and 11 */
    GPIOB->AFRH &= ~((0xFU << 8) | (0xFU << 12));
    GPIOB->AFRH |=  ((4U << 8) | (4U << 12));
}

void I2C2_Init(void)
{
    RCC_APB1ENR1 |= I2C2_EN;

    I2C2_CR1 &= ~I2C_PE; // disable before programming TIMINGR
    I2C2_TIMINGR = I2C2_TIMING_100K;
    I2C2_CR1 |= I2C_PE;
}

int I2C2_Write(uint8_t addr, uint8_t reg, uint8_t data)
{
    /* Write register address + data (2 bytes), AUTOEND generates STOP */
    I2C2_CR2 = ((uint32_t)addr << 1) |
               (2U << 16) |
               I2C_AUTOEND |
               I2C_START;

    if (I2C2_Wait_Flag(I2C_TXIS, 1) != 0)
    {
        return -1;
    }
    I2C2_TXDR = reg;

    if (I2C2_Wait_Flag(I2C_TXIS, 1) != 0)
    {
        return -1;
    }
    I2C2_TXDR = data;

    if (I2C2_Wait_Flag(I2C_STOPF, 1) != 0)
    {
        return -1;
    }
    I2C2_ICR = I2C_STOPCF;
    return 0;
}

int I2C2_Read(uint8_t addr, uint8_t reg, uint8_t *data)
{
    /* Phase 1: write register address (no AUTOEND, then restart) */
    I2C2_CR2 = ((uint32_t)addr << 1) |
               (1U << 16) |
               I2C_START;

    if (I2C2_Wait_Flag(I2C_TXIS, 1) != 0)
    {
        return -1;
    }
    I2C2_TXDR = reg;

    if (I2C2_Wait_Flag(I2C_TC, 1) != 0)
    {
        return -1;
    }

    /* Phase 2: read 1 byte with AUTOEND */
    I2C2_CR2 = ((uint32_t)addr << 1) |
               (1U << 16) |
               I2C_RD_WRN |
               I2C_AUTOEND |
               I2C_START;

    if (I2C2_Wait_Flag(I2C_RXNE, 1) != 0)
    {
        return -1;
    }
    *data = (uint8_t)I2C2_RXDR;

    if (I2C2_Wait_Flag(I2C_STOPF, 1) != 0)
    {
        return -1;
    }
    I2C2_ICR = I2C_STOPCF;
    return 0;
}

int I2C2_Read_Multi(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    /* Write starting register */
    I2C2_CR2 = ((uint32_t)addr << 1) |
               (1U << 16) |
               I2C_START;

    if (I2C2_Wait_Flag(I2C_TXIS, 1) != 0)
    {
        return -1;
    }
    I2C2_TXDR = reg;

    if (I2C2_Wait_Flag(I2C_TC, 1) != 0)
    {
        return -1;
    }

    /* Read len bytes */
    I2C2_CR2 = ((uint32_t)addr << 1) |
               ((uint32_t)len << 16) |
               I2C_RD_WRN |
               I2C_AUTOEND |
               I2C_START;

    for (i = 0; i < len; i++)
    {
        if (I2C2_Wait_Flag(I2C_RXNE, 1) != 0)
        {
            return -1;
        }
        buf[i] = (uint8_t)I2C2_RXDR;
    }

    if (I2C2_Wait_Flag(I2C_STOPF, 1) != 0)
    {
        return -1;
    }
    I2C2_ICR = I2C_STOPCF;
    return 0;
}

int LSM6DSL_Init(void)
{
    uint8_t who = 0;

    if (I2C2_Read(LSM6DSL_ADDR, LSM6DSL_WHO_AM_I_REG, &who) != 0)
    {
        return -1;
    }
    if (who != LSM6DSL_WHO_AM_I_VAL)
    {
        return -1;
    }

    /* Exit power-down, set ODR/range, enable auto-increment */
    if (I2C2_Write(LSM6DSL_ADDR, LSM6DSL_CTRL3_C, LSM6DSL_CTRL3_C_VAL) != 0)
    {
        return -1;
    }
    if (I2C2_Write(LSM6DSL_ADDR, LSM6DSL_CTRL1_XL, LSM6DSL_CTRL1_XL_VAL) != 0)
    {
        return -1;
    }
    if (I2C2_Write(LSM6DSL_ADDR, LSM6DSL_CTRL2_G, LSM6DSL_CTRL2_G_VAL) != 0)
    {
        return -1;
    }

    return (int)who;
}

int LSM6DSL_Read_Accel(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t raw[12];

    /* 12 bytes: G XYZ then XL XYZ starting at OUTX_L_G */
    if (I2C2_Read_Multi(LSM6DSL_ADDR, LSM6DSL_OUTX_L_G, raw, 12) != 0)
    {
        return -1;
    }

    *ax = (int16_t)((uint16_t)raw[6]  | ((uint16_t)raw[7]  << 8));
    *ay = (int16_t)((uint16_t)raw[8]  | ((uint16_t)raw[9]  << 8));
    *az = (int16_t)((uint16_t)raw[10] | ((uint16_t)raw[11] << 8));
    return 0;
}

static int32_t iabs32(int32_t v)
{
    return (v < 0) ? -v : v;
}

uint32_t Gesture_Classify(void)
{
    uint32_t i;
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    int32_t mean_x;
    int32_t mean_y;
    int32_t mean_z;
    int64_t energy = 0;
    int32_t peak = 0;
    int32_t dx;
    int32_t dy;
    int32_t dz;
    int32_t mag;

    for (i = 0; i < BUF_LEN; i++)
    {
        sum_x += ax_buf[i];
        sum_y += ay_buf[i];
        sum_z += az_buf[i];
    }

    mean_x = sum_x / (int32_t)BUF_LEN;
    mean_y = sum_y / (int32_t)BUF_LEN;
    mean_z = sum_z / (int32_t)BUF_LEN;

    /* Fixed-point high-pass: remove window mean (gravity / bias) */
    for (i = 0; i < BUF_LEN; i++)
    {
        dx = (int32_t)ax_buf[i] - mean_x;
        dy = (int32_t)ay_buf[i] - mean_y;
        dz = (int32_t)az_buf[i] - mean_z;

        energy += (int64_t)dx * dx + (int64_t)dy * dy + (int64_t)dz * dz;

        mag = iabs32(dx) + iabs32(dy) + iabs32(dz);
        if (mag > peak)
        {
            peak = mag;
        }
    }

    /* shake: high AC energy */
    if (energy > SHAKE_ENERGY_THR)
    {
        return 1U;
    }

    /* tilt: large DC mean on X or Y (board tipped) */
    if ((iabs32(mean_x) > TILT_MEAN_THR) || (iabs32(mean_y) > TILT_MEAN_THR))
    {
        return 2U;
    }

    (void)peak; /* available for future threshold tuning */
    return 0U;  /* still */
}

int main(void)
{
    int who;
    int16_t ax;
    int16_t ay;
    int16_t az;
    uint32_t gesture_id = 0;
    uint32_t last_report_ms = 0;

    UART4_GPIO_Init();
    UART_Setup();
    SysTick_Init();

    I2C2_GPIO_Init();
    I2C2_Init();

    who = LSM6DSL_Init();
    /* Emit WHO_AM_I so the host can confirm the bus (expect 0x6A) */
    Convert_To_Bytes((who > 0) ? (uint32_t)who : 0U);

    while (1)
    {
        if (sample_flag)
        {
            sample_flag = 0U;

            if (LSM6DSL_Read_Accel(&ax, &ay, &az) == 0)
            {
                ax_buf[buf_idx] = ax;
                ay_buf[buf_idx] = ay;
                az_buf[buf_idx] = az;
                buf_idx++;
                if (buf_idx >= BUF_LEN)
                {
                    buf_idx = 0U;
                    buf_filled = 1U;
                }
            }
        }

        /* Classify once the circular buffer has a full window */
        if (buf_filled && ((timer - last_report_ms) >= (SAMPLE_PERIOD_MS * BUF_LEN)))
        {
            last_report_ms = timer;
            gesture_id = Gesture_Classify();
            Convert_To_Bytes(gesture_id);
            Convert_To_Bytes(timer);
        }
    }
    return 0; // never reached
}
