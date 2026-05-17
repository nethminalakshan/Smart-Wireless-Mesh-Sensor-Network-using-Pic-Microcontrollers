#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Embedded C equivalents for Arduino libraries
// Hardware-specific implementations needed for your MCU

// SPI for RF24 radio
void SPI_Init(void);
void SPI_Transfer(uint8_t* tx_data, uint8_t* rx_data, uint8_t length);

// I2C for LCD
void I2C_Init(void);
void I2C_WriteByte(uint8_t device_addr, uint8_t reg_addr, uint8_t data);
void I2C_WriteBytes(uint8_t device_addr, uint8_t reg_addr, uint8_t* data, uint8_t length);

// RF24 radio module (simplified)
typedef struct {
    uint8_t pipe_address[6];
    bool listening;
} RF24;

void RF24_Init(RF24* radio);
void RF24_SetDataRate(RF24* radio, uint32_t rate);
void RF24_SetChannel(RF24* radio, uint8_t channel);
void RF24_SetPALevel(RF24* radio, uint8_t level);
void RF24_OpenReadingPipe(RF24* radio, uint8_t pipe_num, const uint8_t* address);
void RF24_StartListening(RF24* radio);
bool RF24_Available(RF24* radio);
void RF24_Read(RF24* radio, void* data, uint8_t length);

// LCD 16x2 with I2C backpack
#define LCD_ADDRESS 0x27
#define LCD_WIDTH 16
#define LCD_HEIGHT 2

// LCD commands
#define LCD_CLEAR 0x01
#define LCD_RETURN_HOME 0x02
#define LCD_ENTRY_MODE 0x06
#define LCD_DISPLAY_ON 0x0C
#define LCD_FUNCTION_4BIT 0x28
#define LCD_SET_CURSOR 0x80

void LCD_Init(void);
void LCD_SendCommand(uint8_t cmd);
void LCD_SendData(uint8_t data);
void LCD_SetCursor(uint8_t col, uint8_t row);
void LCD_Print(const char* str);
void LCD_PrintChar(char c);
void LCD_Clear(void);
void LCD_Backlight(bool on);

// GPIO functions
void GPIO_Init(void);
void GPIO_WritePin(uint8_t pin, bool state);
bool GPIO_ReadPin(uint8_t pin);
void GPIO_SetPinMode(uint8_t pin, bool is_output);

// Timing functions
uint32_t millis(void);
void delay_ms(uint32_t ms);

// Pin definitions
#define LED_PIN 6

// Radio constants
#define RF24_250KBPS 250000
#define RF24_PA_LOW 0

// LCD pages
#define PAGE_TEMP_HUM_GAS 0
#define PAGE_VIB_FLAME 1

// Sensor data structure (MUST MATCH NODE B)
typedef struct {
    uint8_t nodeID;
    float temp;
    float hum;
    int16_t gas;
    int16_t vibration;
    int16_t flame;
} SensorData;

// Global variables
static RF24 radio;
static SensorData data;
static int page = 0;
static uint32_t lastChange = 0;
static bool led_state = false;

// Buffer for LCD display
static char lcd_buffer[17];

int main(void) {
    // System initialization
    GPIO_Init();
    SPI_Init();
    I2C_Init();
    
    // Configure LED pin
    GPIO_SetPinMode(LED_PIN, true);
    GPIO_WritePin(LED_PIN, false);
    
    // Initialize LCD
    LCD_Init();
    LCD_Backlight(true);
    LCD_Clear();
    
    // Initialize radio
    RF24_Init(&radio);
    RF24_SetDataRate(&radio, RF24_250KBPS);
    RF24_SetChannel(&radio, 76);
    RF24_SetPALevel(&radio, RF24_PA_LOW);
    RF24_OpenReadingPipe(&radio, 0, (const uint8_t*)"NODEC");
    RF24_StartListening(&radio);
    
    // Main loop
    while (1) {
        // Check for incoming radio data
        if (RF24_Available(&radio)) {
            RF24_Read(&radio, &data, sizeof(data));
            
            // LED ON while receiving
            GPIO_WritePin(LED_PIN, true);
            
            // Print to serial (assuming you have serial output)
            // In embedded C, you'd implement your own serial output
            print_serial("\n========= FROM NODE B =========\n");
            print_serial("NODE: ");
            print_serial_int(data.nodeID);
            print_serial("\nTEMP: ");
            print_serial_float(data.temp);
            print_serial("\nHUM : ");
            print_serial_float(data.hum);
            print_serial("\nGAS : ");
            print_serial_int(data.gas);
            print_serial("\nVIB : ");
            print_serial_int(data.vibration);
            print_serial("\nFLAME: ");
            print_serial_int(data.flame);
            print_serial("\n===============================\n\n");
        }
        
        // Update LCD display (page changes every 2 seconds)
        if (millis() - lastChange > 2000) {
            page++;
            if (page > 1) page = 0;
            lastChange = millis();
            LCD_Clear();
        }
        
        if (page == PAGE_TEMP_HUM_GAS) {
            // Page 0: Temperature, Humidity, Gas
            LCD_SetCursor(0, 0);
            snprintf(lcd_buffer, sizeof(lcd_buffer), "T:%.1f H:%.0f", data.temp, data.hum);
            LCD_Print(lcd_buffer);
            
            LCD_SetCursor(0, 1);
            snprintf(lcd_buffer, sizeof(lcd_buffer), "G:%d", data.gas);
            LCD_Print(lcd_buffer);
        } 
        else {
            // Page 1: Vibration, Flame
            LCD_SetCursor(0, 0);
            snprintf(lcd_buffer, sizeof(lcd_buffer), "V:%d", data.vibration);
            LCD_Print(lcd_buffer);
            
            LCD_SetCursor(0, 1);
            snprintf(lcd_buffer, sizeof(lcd_buffer), "F:%d", data.flame);
            LCD_Print(lcd_buffer);
        }
        
        delay_ms(50);
        GPIO_WritePin(LED_PIN, false);
    }
    
    return 0;
}

// ============ LCD I2C IMPLEMENTATION ============
// For PCF8574 I2C backpack

static uint8_t lcd_backlight_state = 0x08; // Backlight ON

void LCD_WriteNibble(uint8_t nibble, uint8_t mode) {
    uint8_t data = (nibble << 4) | mode | lcd_backlight_state;
    I2C_WriteByte(LCD_ADDRESS, 0, data);
    delay_ms(1);
    
    // Pulse enable pin
    data |= 0x04;  // EN high
    I2C_WriteByte(LCD_ADDRESS, 0, data);
    delay_ms(1);
    
    data &= ~0x04; // EN low
    I2C_WriteByte(LCD_ADDRESS, 0, data);
    delay_ms(1);
}

void LCD_SendCommand(uint8_t cmd) {
    uint8_t high_nibble = (cmd >> 4) & 0x0F;
    uint8_t low_nibble = cmd & 0x0F;
    
    LCD_WriteNibble(high_nibble, 0x00); // RS = 0 for command
    LCD_WriteNibble(low_nibble, 0x00);
}

void LCD_SendData(uint8_t data) {
    uint8_t high_nibble = (data >> 4) & 0x0F;
    uint8_t low_nibble = data & 0x0F;
    
    LCD_WriteNibble(high_nibble, 0x01); // RS = 1 for data
    LCD_WriteNibble(low_nibble, 0x01);
}

void LCD_Init(void) {
    delay_ms(50);
    
    // Initialization sequence for 4-bit mode
    LCD_WriteNibble(0x03, 0x00);
    delay_ms(5);
    LCD_WriteNibble(0x03, 0x00);
    delay_ms(1);
    LCD_WriteNibble(0x03, 0x00);
    delay_ms(1);
    LCD_WriteNibble(0x02, 0x00); // Set to 4-bit mode
    
    LCD_SendCommand(LCD_FUNCTION_4BIT);  // 2 lines, 5x8 font
    LCD_SendCommand(LCD_DISPLAY_ON);     // Display on, cursor off
    LCD_SendCommand(LCD_CLEAR);          // Clear display
    LCD_SendCommand(LCD_ENTRY_MODE);     // Increment cursor
    delay_ms(2);
}

void LCD_SetCursor(uint8_t col, uint8_t row) {
    uint8_t address;
    
    switch(row) {
        case 0: address = 0x00; break;
        case 1: address = 0x40; break;
        default: address = 0x00;
    }
    
    address += col;
    LCD_SendCommand(LCD_SET_CURSOR | address);
}

void LCD_Print(const char* str) {
    while (*str) {
        LCD_SendData(*str++);
    }
}

void LCD_PrintChar(char c) {
    LCD_SendData(c);
}

void LCD_Clear(void) {
    LCD_SendCommand(LCD_CLEAR);
    delay_ms(2);
}

void LCD_Backlight(bool on) {
    if (on) {
        lcd_backlight_state = 0x08;
    } else {
        lcd_backlight_state = 0x00;
    }
    
    // Update backlight without changing display
    I2C_WriteByte(LCD_ADDRESS, 0, lcd_backlight_state);
}

// ============ HARDWARE-SPECIFIC IMPLEMENTATIONS ============
// Example for STM32F103 (Blue Pill)

#ifdef STM32F1

#include "stm32f1xx_hal.h"

// I2C handle
I2C_HandleTypeDef hi2c1;

// Timer for millis()
volatile uint32_t system_ms = 0;

void SysTick_Handler(void) {
    system_ms++;
}

uint32_t millis(void) {
    return system_ms;
}

void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

// GPIO initialization
void GPIO_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // LED on PB6
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void GPIO_SetPinMode(uint8_t pin, bool is_output) {
    // Implementation depends on your pin mapping
    // For simplicity, assuming PB6 for LED
}

void GPIO_WritePin(uint8_t pin, bool state) {
    if (pin == LED_PIN) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

// I2C initialization
void I2C_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // PB6 (SCL), PB7 (SDA)
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

void I2C_WriteByte(uint8_t device_addr, uint8_t reg_addr, uint8_t data) {
    HAL_I2C_Mem_Write(&hi2c1, device_addr << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

// SPI initialization (for RF24)
void SPI_Init(void) {
    // Implementation for SPI1 on STM32
    // Pins: PA4 (CS), PA5 (SCK), PA6 (MISO), PA7 (MOSI)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    
    // Configure SPI pins
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7; // SCK, MOSI
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_6; // MISO
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // CS pin as output
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

// Serial output (UART) for debugging
void print_serial(const char* str) {
    // Implement UART output
    while (*str) {
        // Send character via UART
        str++;
    }
}

void print_serial_int(int value) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", value);
    print_serial(buffer);
}

void print_serial_float(float value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    print_serial(buffer);
}

#endif // STM32F1