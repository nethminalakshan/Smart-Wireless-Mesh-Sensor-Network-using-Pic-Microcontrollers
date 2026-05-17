#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

// Embedded C equivalents for Arduino libraries
// Hardware-specific implementations needed for your MCU

// SPI for RF24 radio
void SPI_Init(void);
void SPI_Transfer(uint8_t* tx_data, uint8_t* rx_data, uint8_t length);

// GPIO functions
void GPIO_Init(void);
void GPIO_WritePin(uint8_t pin, bool state);
bool GPIO_ReadPin(uint8_t pin);
void GPIO_SetPinMode(uint8_t pin, bool is_output);

// ADC for gas sensor
void ADC_Init(void);
uint16_t ADC_Read(uint8_t channel);

// DHT11 sensor
void DHT_Init(void);
bool DHT_Read(float* temperature, float* humidity);

// Timing functions
void delay_ms(uint32_t ms);
uint32_t millis(void);

// Serial output (for debugging)
void Serial_Init(uint32_t baudrate);
void Serial_Print(const char* str);
void Serial_Println(const char* str);
void Serial_PrintInt(int value);
void Serial_PrintFloat(float value);

// RF24 radio module (simplified)
typedef struct {
    uint8_t pipe_address[6];
    bool listening;
    uint8_t retries_delay;
    uint8_t retries_count;
} RF24;

void RF24_Init(RF24* radio);
void RF24_SetDataRate(RF24* radio, uint32_t rate);
void RF24_SetChannel(RF24* radio, uint8_t channel);
void RF24_SetPALevel(RF24* radio, uint8_t level);
void RF24_SetRetries(RF24* radio, uint8_t delay, uint8_t count);
void RF24_OpenWritingPipe(RF24* radio, const uint8_t* address);
void RF24_StopListening(RF24* radio);
bool RF24_Write(RF24* radio, const void* data, uint8_t length);

// Pin definitions
#define DHTPIN 2
#define VIB_PIN 3
#define FLAME_PIN 4
#define GAS_AO_PIN 0  // A0 analog channel
#define LED_PIN 6

// Radio constants
#define RF24_250KBPS 250000
#define RF24_PA_LOW 0

// DHT11 constants
#define DHT11_START_LOW_US 18000
#define DHT11_START_HIGH_US 40
#define DHT11_BIT_LOW_US 28
#define DHT11_BIT_HIGH_US 70

// Sensor thresholds
#define GAS_HIGH_THRESHOLD 400

// Address for Node C
static const uint8_t addressC[6] = "NODEC";

// Sensor data structure (MUST MATCH NODE C)
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
static float lastTemp = 0;
static float lastHum = 0;
static int16_t gasSmooth = 0;

// Simple delay function (busy wait)
// For production, use hardware timer
void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 1000; i++) {
        __asm__("nop");
    }
}

uint32_t millis(void) {
    // In production, use a hardware timer
    static uint32_t counter = 0;
    delay_ms(1);
    return counter++;
}

// DHT11 timing-critical functions
static bool dht_wait_for_level(bool level, uint32_t timeout_us) {
    uint32_t start = millis();
    while (GPIO_ReadPin(DHTPIN) != level) {
        if ((millis() - start) > timeout_us / 1000) return false;
    }
    return true;
}

static uint8_t dht_read_byte(void) {
    uint8_t value = 0;
    
    for (int i = 0; i < 8; i++) {
        // Wait for the start of the bit (low to high transition)
        while (GPIO_ReadPin(DHTPIN) == 0);
        
        // Measure the high pulse width
        uint32_t start = micros();
        while (GPIO_ReadPin(DHTPIN) == 1);
        uint32_t duration = micros() - start;
        
        // If pulse width > 50us, it's a '1', else '0'
        if (duration > 50) {
            value |= (1 << (7 - i));
        }
    }
    
    return value;
}

// DHT11 read function (simplified - needs precise timing)
bool DHT_Read(float* temperature, float* humidity) {
    uint8_t data[5] = {0};
    
    // Send start signal
    GPIO_SetPinMode(DHTPIN, true);
    GPIO_WritePin(DHTPIN, false);
    delay_ms(18);  // 18ms low pulse
    
    GPIO_WritePin(DHTPIN, true);
    delay_ms(1);   // 1ms high
    
    GPIO_SetPinMode(DHTPIN, false);  // Switch to input
    
    // Wait for DHT11 response
    delay_ms(1);
    
    // Check for response pulse (80us low then 80us high)
    if (!dht_wait_for_level(false, 100)) return false;
    if (!dht_wait_for_level(true, 100)) return false;
    
    // Read 40 bits (5 bytes)
    for (int i = 0; i < 5; i++) {
        data[i] = dht_read_byte();
    }
    
    // Verify checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (data[4] != checksum) return false;
    
    *humidity = (float)data[0];
    *temperature = (float)data[2];
    
    return true;
}

// ADC read for gas sensor
uint16_t ADC_Read(uint8_t channel) {
    // Placeholder - implement based on your MCU's ADC
    // For STM32, use HAL_ADC_GetValue()
    return 256;  // Mock value
}

// Simple exponential smoothing for gas sensor
int16_t smooth_gas(int16_t new_value, int16_t previous_smooth) {
    return (previous_smooth * 3 + new_value) / 4;
}

// Serial output functions (UART)
void Serial_Init(uint32_t baudrate) {
    // Initialize UART for debugging
    // Implementation depends on your MCU
}

void Serial_Print(const char* str) {
    while (*str) {
        // Send character via UART
        str++;
    }
}

void Serial_Println(const char* str) {
    Serial_Print(str);
    Serial_Print("\r\n");
}

void Serial_PrintInt(int value) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", value);
    Serial_Print(buffer);
}

void Serial_PrintFloat(float value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    Serial_Print(buffer);
}

int main(void) {
    // System initialization
    GPIO_Init();
    ADC_Init();
    SPI_Init();
    Serial_Init(9600);
    
    // Configure pins
    GPIO_SetPinMode(VIB_PIN, false);   // Input
    GPIO_SetPinMode(FLAME_PIN, false); // Input
    GPIO_SetPinMode(LED_PIN, true);    // Output
    GPIO_WritePin(LED_PIN, false);
    
    // Initialize DHT11
    DHT_Init();
    GPIO_SetPinMode(DHTPIN, false);    // Start as input
    
    // Initialize radio
    RF24_Init(&radio);
    RF24_SetDataRate(&radio, RF24_250KBPS);
    RF24_SetChannel(&radio, 76);
    RF24_SetPALevel(&radio, RF24_PA_LOW);
    RF24_SetRetries(&radio, 15, 15);
    RF24_OpenWritingPipe(&radio, addressC);
    RF24_StopListening(&radio);
    
    Serial_Println("=== Sensor System Started (Node B) ===");
    
    // Gas sensor warm-up delay (20 seconds)
    delay_ms(20000);
    
    // Main loop
    while (1) {
        float temp = 0, hum = 0;
        bool dht_ok = DHT_Read(&temp, &hum);
        
        uint16_t gasValue = ADC_Read(GAS_AO_PIN);
        uint8_t vibration = GPIO_ReadPin(VIB_PIN);
        uint8_t flame = GPIO_ReadPin(FLAME_PIN);
        
        Serial_Println("------ Sensor Data ------");
        
        // DHT11 data handling
        if (!dht_ok) {
            Serial_Println("DHT11 Error!");
            // Use previous values if DHT fails
            temp = lastTemp;
            hum = lastHum;
        } else {
            lastTemp = temp;
            lastHum = hum;
            
            Serial_Print("Temperature: ");
            Serial_PrintFloat(temp);
            Serial_Println(" °C");
            
            Serial_Print("Humidity: ");
            Serial_PrintFloat(hum);
            Serial_Println(" %");
        }
        
        // Gas sensor with smoothing
        gasSmooth = smooth_gas((int16_t)gasValue, gasSmooth);
        
        Serial_Print("Gas Value: ");
        Serial_PrintInt(gasSmooth);
        Serial_Println("");
        
        if (gasSmooth > GAS_HIGH_THRESHOLD) {
            Serial_Println("⚠️ Gas Level HIGH!");
        } else {
            Serial_Println("Gas Level Normal");
        }
        
        // Flame sensor (active LOW)
        Serial_Print("Flame: ");
        if (flame == 0) {  // LOW means fire detected
            Serial_Println("🔥 FIRE DETECTED!");
        } else {
            Serial_Println("No Fire");
        }
        
        // Vibration sensor
        Serial_Print("Vibration: ");
        if (vibration == 1) {
            Serial_Println("📳 Vibration Detected!");
        } else {
            Serial_Println("No Vibration");
        }
        
        Serial_Println("--------------------------\n");
        
        // Prepare data packet
        data.nodeID = 2;
        data.temp = lastTemp;
        data.hum = lastHum;
        data.gas = gasSmooth;
        data.vibration = vibration;
        data.flame = flame;
        
        // Send to Node C
        GPIO_WritePin(LED_PIN, true);
        RF24_Write(&radio, &data, sizeof(data));
        delay_ms(50);
        GPIO_WritePin(LED_PIN, false);
        
        delay_ms(1500);  // 1.5 second interval
    }
    
    return 0;
}

// ============ HARDWARE-SPECIFIC IMPLEMENTATIONS ============
// Example for STM32F103 (Blue Pill)

#ifdef STM32F1

#include "stm32f1xx_hal.h"

// ADC handle
ADC_HandleTypeDef hadc1;

// GPIO initialization
void GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // LED on PA6
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // VIB (PA3), FLAME (PA4), DHT (PA2) as inputs
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void GPIO_SetPinMode(uint8_t pin, bool is_output) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = 1 << pin;
    
    if (is_output) {
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    } else {
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }
    
    if (pin < 8) {
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    } else {
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

void GPIO_WritePin(uint8_t pin, bool state) {
    GPIO_TypeDef* port = (pin < 8) ? GPIOA : GPIOB;
    uint16_t pin_mask = 1 << (pin % 8);
    HAL_GPIO_WritePin(port, pin_mask, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool GPIO_ReadPin(uint8_t pin) {
    GPIO_TypeDef* port = (pin < 8) ? GPIOA : GPIOB;
    uint16_t pin_mask = 1 << (pin % 8);
    return (HAL_GPIO_ReadPin(port, pin_mask) == GPIO_PIN_SET);
}

// ADC initialization
void ADC_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    
    // Configure PA0 as analog input for gas sensor
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // ADC configuration
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);
}

uint16_t ADC_Read(uint8_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;  // PA0
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint16_t value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    
    return value;
}

// DHT11 initialization (nothing specific needed)
void DHT_Init(void) {
    // Set pin as output for initialization
    GPIO_SetPinMode(DHTPIN, true);
    GPIO_WritePin(DHTPIN, true);
}

// SPI initialization for RF24
void SPI_Init(void) {
    // Implementation for SPI1
    // Pins: PA4 (CS), PA5 (SCK), PA6 (MISO), PA7 (MOSI)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // SCK, MOSI as alternate function
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // MISO as input
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // CS as output
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    
    // SPI configuration
    SPI_HandleTypeDef hspi1;
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    HAL_SPI_Init(&hspi1);
}

#endif // STM32F1

// Microsecond delay (for DHT11 precise timing)
uint32_t micros(void) {
    // For STM32, use SysTick or TIM2
    // Simplified: return millis() * 1000
    return millis() * 1000;
}