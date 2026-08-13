#include "main.h"


/* =====================================================
   I2C LCD ADDRESS
   ===================================================== */

#define LCD_ADDR    0x27


/* =====================================================
   LCD CONTROL BITS
   ===================================================== */

#define LCD_RS      (1 << 0)
#define LCD_EN      (1 << 2)
#define LCD_BL      (1 << 3)


/* =====================================================
   FUNCTION PROTOTYPES
   ===================================================== */

void I2C1_GPIO_Init(void);
void I2C1_Init(void);

uint8_t I2C1_CheckAddress(uint8_t address);
void I2C1_Stop(void);

void LCD_Init(void);
void LCD_SendCommand(uint8_t command);
void LCD_SendData(uint8_t data);
void LCD_SendNibble(uint8_t nibble, uint8_t control);

void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t column);
void LCD_Print(char *text);
void LCD_PrintHex(uint8_t value);

void delay_ms(uint32_t ms);


/* =====================================================
   MAIN
   ===================================================== */

int main(void)
{
    uint8_t addresses[] =
    {
        0x27,
        0x42,
        0x31,
        0x50
    };

    uint8_t i;
    uint8_t result;


    /* Initialize GPIO */

    I2C1_GPIO_Init();


    /* Initialize I2C */

    I2C1_Init();


    /* Initialize LCD */

    LCD_Init();


    /* =================================================
       CONTINUOUS ACK/NACK TEST
       ================================================= */

    while(1)
    {
        for(i = 0; i < 4; i++)
        {
            /*
             * Check address
             */

            result = I2C1_CheckAddress(addresses[i]);


            /*
             * Clear LCD
             */

            LCD_Clear();


            /*
             * Display address
             */

            LCD_SetCursor(0, 0);

            LCD_Print("Address: 0x");

            LCD_PrintHex(addresses[i]);


            /*
             * Display ACK or NACK
             */

            LCD_SetCursor(1, 0);


            if(result == 1)
            {
                LCD_Print("ACK");
            }
            else
            {
                LCD_Print("NACK");
            }


            /*
             * Keep result visible
             * for approximately 0.5 second
             */

            delay_ms(500);
        }
    }
}


/* =====================================================
   I2C GPIO INITIALIZATION
   ===================================================== */

void I2C1_GPIO_Init(void)
{
    /*
     * Enable GPIOB clock
     */

    RCC->AHB1ENR |= (1 << 1);


    /*
     * PB6 -> Alternate Function
     */

    GPIOB->MODER &= ~(3 << (6 * 2));
    GPIOB->MODER |=  (2 << (6 * 2));


    /*
     * PB7 -> Alternate Function
     */

    GPIOB->MODER &= ~(3 << (7 * 2));
    GPIOB->MODER |=  (2 << (7 * 2));


    /*
     * Open drain
     */

    GPIOB->OTYPER |= (1 << 6);
    GPIOB->OTYPER |= (1 << 7);


    /*
     * High speed
     */

    GPIOB->OSPEEDR |= (3 << (6 * 2));
    GPIOB->OSPEEDR |= (3 << (7 * 2));


    /*
     * No internal pull-up
     */

    GPIOB->PUPDR &= ~(3 << (6 * 2));
    GPIOB->PUPDR &= ~(3 << (7 * 2));


    /*
     * PB6 -> AF4 -> I2C1_SCL
     */

    GPIOB->AFR[0] &= ~(0xF << (6 * 4));
    GPIOB->AFR[0] |=  (4 << (6 * 4));


    /*
     * PB7 -> AF4 -> I2C1_SDA
     */

    GPIOB->AFR[0] &= ~(0xF << (7 * 4));
    GPIOB->AFR[0] |=  (4 << (7 * 4));
}


/* =====================================================
   I2C1 INITIALIZATION
   ===================================================== */

void I2C1_Init(void)
{
    /*
     * Enable I2C1 clock
     */

    RCC->APB1ENR |= (1 << 21);


    /*
     * Reset I2C1
     */

    I2C1->CR1 |= (1 << 15);

    I2C1->CR1 &= ~(1 << 15);


    /*
     * APB1 clock = 16 MHz
     */

    I2C1->CR2 = 16;


    /*
     * 100 kHz I2C
     */

    I2C1->CCR = 80;


    /*
     * Maximum rise time
     */

    I2C1->TRISE = 17;


    /*
     * Enable I2C1
     */

    I2C1->CR1 |= (1 << 0);
}


/* =====================================================
   CHECK I2C ADDRESS
   ===================================================== */

uint8_t I2C1_CheckAddress(uint8_t address)
{
    uint32_t timeout;
    volatile uint32_t temp;


    /*
     * Generate START
     */

    I2C1->CR1 |= (1 << 8);


    /*
     * Wait for START condition
     */

    timeout = 100000;

    while(!(I2C1->SR1 & (1 << 0)))
    {
        timeout--;

        if(timeout == 0)
        {
            I2C1_Stop();

            return 0;
        }
    }


    /*
     * Send device address
     *
     * Shift left by one.
     *
     * Bit 0 = 0
     * because this is a WRITE operation.
     */

    I2C1->DR = (address << 1);


    /*
     * Wait for:
     *
     * ADDR = ACK
     *
     * AF = NACK
     */

    timeout = 100000;

    while(!(I2C1->SR1 & ((1 << 1) | (1 << 10))))
    {
        timeout--;

        if(timeout == 0)
        {
            I2C1_Stop();

            return 0;
        }
    }


    /* =================================================
       ACK RECEIVED
       ================================================= */

    if(I2C1->SR1 & (1 << 1))
    {
        /*
         * Clear ADDR flag
         *
         * Read SR1 then SR2
         */

        temp = I2C1->SR1;

        temp = I2C1->SR2;

        (void)temp;


        /*
         * Generate STOP
         */

        I2C1_Stop();


        return 1;
    }


    /* =================================================
       NACK RECEIVED
       ================================================= */

    if(I2C1->SR1 & (1 << 10))
    {
        /*
         * Clear AF flag
         */

        I2C1->SR1 &= ~(1 << 10);


        /*
         * Generate STOP
         */

        I2C1_Stop();


        return 0;
    }


    return 0;
}


/* =====================================================
   I2C STOP
   ===================================================== */

void I2C1_Stop(void)
{
    /*
     * Generate STOP condition
     */

    I2C1->CR1 |= (1 << 9);
}


/* =====================================================
   LCD SEND NIBBLE
   ===================================================== */

void LCD_SendNibble(uint8_t nibble, uint8_t control)
{
    uint8_t data;
    volatile uint32_t temp;


    /*
     * Put 4-bit LCD data
     * on bits 4-7
     */

    data = ((nibble & 0x0F) << 4);


    /*
     * Add RS
     */

    data |= control;


    /*
     * Turn backlight ON
     */

    data |= LCD_BL;


    /*
     * START
     */

    I2C1->CR1 |= (1 << 8);


    /*
     * Wait for START
     */

    while(!(I2C1->SR1 & (1 << 0)))
    {
    }


    /*
     * Send LCD I2C address
     */

    I2C1->DR = (LCD_ADDR << 1);


    /*
     * Wait for ACK
     */

    while(!(I2C1->SR1 & ((1 << 1) | (1 << 10))))
    {
    }


    /*
     * Clear ADDR
     */

    temp = I2C1->SR1;

    temp = I2C1->SR2;

    (void)temp;


    /*
     * Send data
     */

    I2C1->DR = data;

    while(!(I2C1->SR1 & (1 << 2)))
    {
    }


    /*
     * Enable HIGH
     */

    I2C1->DR = data | LCD_EN;

    while(!(I2C1->SR1 & (1 << 2)))
    {
    }


    delay_ms(1);


    /*
     * Enable LOW
     */

    I2C1->DR = data;

    while(!(I2C1->SR1 & (1 << 2)))
    {
    }


    /*
     * STOP
     */

    I2C1_Stop();


    delay_ms(1);
}


/* =====================================================
   LCD COMMAND
   ===================================================== */

void LCD_SendCommand(uint8_t command)
{
    LCD_SendNibble(command >> 4, 0);

    LCD_SendNibble(command & 0x0F, 0);

    delay_ms(2);
}


/* =====================================================
   LCD DATA
   ===================================================== */

void LCD_SendData(uint8_t data)
{
    LCD_SendNibble(data >> 4, LCD_RS);

    LCD_SendNibble(data & 0x0F, LCD_RS);

    delay_ms(2);
}


/* =====================================================
   LCD INITIALIZATION
   ===================================================== */

void LCD_Init(void)
{
    delay_ms(50);


    /*
     * Initialize LCD
     * into 4-bit mode
     */

    LCD_SendNibble(0x03, 0);

    delay_ms(5);

    LCD_SendNibble(0x03, 0);

    delay_ms(5);

    LCD_SendNibble(0x03, 0);

    delay_ms(5);

    LCD_SendNibble(0x02, 0);


    /*
     * 4-bit mode
     * 2 lines
     * 5x8 font
     */

    LCD_SendCommand(0x28);


    /*
     * Display ON
     * Cursor OFF
     */

    LCD_SendCommand(0x0C);


    /*
     * Clear display
     */

    LCD_SendCommand(0x01);


    /*
     * Entry mode
     */

    LCD_SendCommand(0x06);
}


/* =====================================================
   LCD CLEAR
   ===================================================== */

void LCD_Clear(void)
{
    LCD_SendCommand(0x01);

    delay_ms(2);
}


/* =====================================================
   LCD SET CURSOR
   ===================================================== */

void LCD_SetCursor(uint8_t row, uint8_t column)
{
    uint8_t address;


    if(row == 0)
    {
        address = 0x00 + column;
    }
    else
    {
        address = 0x40 + column;
    }


    LCD_SendCommand(0x80 | address);
}


/* =====================================================
   LCD PRINT STRING
   ===================================================== */

void LCD_Print(char *text)
{
    while(*text)
    {
        LCD_SendData(*text);

        text++;
    }
}


/* =====================================================
   LCD PRINT HEX
   ===================================================== */

void LCD_PrintHex(uint8_t value)
{
    char hex[] = "0123456789ABCDEF";


    LCD_SendData(hex[(value >> 4) & 0x0F]);

    LCD_SendData(hex[value & 0x0F]);
}


/* =====================================================
   DELAY IN MILLISECONDS
   ===================================================== */

void delay_ms(uint32_t ms)
{
    uint32_t i;


    while(ms--)
    {
        for(i = 0; i < 16000; i++)
        {
            __asm volatile("nop");
        }
    }
}
