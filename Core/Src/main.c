/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ili9341.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
UART_HandleTypeDef huart2;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
static void MX_USART2_UART_Init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern uint32_t SystemCoreClock;

// Global BMS State Variables for TFT Display and Dashboard Pages
volatile uint8_t current_page = 0;
volatile uint8_t page_changed = 1;

float current_pack_v = 0.0f;
float current_pack_a = 0.0f;
float current_soc = 0.0f;
uint8_t current_charge_status = 0; // 0=Idle, 1=Charging, 2=Discharging
volatile uint8_t wifi_connected = 0;
volatile uint8_t mqtt_connected = 0;
float current_t1 = -999.0f;

float current_t2 = -999.0f;
float current_max_vib = 0.0f;
float current_ppm = 0.0f;
float current_c1 = 0.0f;
float current_c2 = 0.0f;
float current_c3 = 0.0f;
float current_c4 = 0.0f;

float pin_c1 = 0.0f;
float pin_c2 = 0.0f;
float pin_c3 = 0.0f;
float pin_c4 = 0.0f;
float pin_curr = 0.0f;
float pin_mq7 = 0.0f;

volatile uint8_t ml_isolation_signal = 1;

volatile uint32_t *DWT_CYCCNT   = (volatile uint32_t *)0xE0001004;
volatile uint32_t *DWT_CONTROL  = (volatile uint32_t *)0xE0001000;
volatile uint32_t *SCB_DEMCR    = (volatile uint32_t *)0xE000EDFC;

void Delay_us(uint32_t us) {
    uint32_t startTick = *DWT_CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000);
    while ((*DWT_CYCCNT - startTick) < delayTicks);
}

#define SCALE_B1 1.4504f
#define SCALE_B2 3.1075f
#define SCALE_B3 4.1822f
#define SCALE_B4 5.5632f

// PB12: Cooling Relay
#define COOLING_RELAY_ON()    (GPIOB->BSRR = (1U << 12)) 
#define COOLING_RELAY_OFF()   (GPIOB->BSRR = (1U << 28)) 
// PB13: Isolation Relay
#define ISOLATION_RELAY_ON()  (GPIOB->BSRR = (1U << 13)) 
#define ISOLATION_RELAY_OFF() (GPIOB->BSRR = (1U << 29)) 

// ACS712-20A Parameters
float ACS712_ZERO_VOLTAGE = 1.241f; // Hardcoded no-load zero current voltage with external power supply
#define ACS712_SENSITIVITY   0.050f  // 50mV/A sensitivity (100mV/A divided by 2 for the 10k/10k divider)

void UART_SendChar(char c) {
    while (!(USART2->SR & USART_SR_TXE)); 
    USART2->DR = c; 
}

void UART_SendString(const char* str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}

void BareMetal_Hardware_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_I2C1EN;

    GPIOA->MODER &= ~(3U << (10 * 2)); 
    GPIOA->PUPDR &= ~(3U << (10 * 2));
    GPIOA->PUPDR |= (1U << (10 * 2));  

    GPIOB->MODER &= ~(3U << (12 * 2) | 3U << (13 * 2));
    GPIOB->MODER |= (1U << (12 * 2) | 1U << (13 * 2));   
    GPIOB->OTYPER &= ~((1U << 12) | (1U << 13));         
    GPIOB->OSPEEDR |= (3U << (12 * 2) | 3U << (13 * 2));  

    GPIOC->MODER |= (3U << (0 * 2)) | (3U << (1 * 2)) | (3U << (2 * 2)) | (3U << (3 * 2)) | (3U << (4 * 2)) | (3U << (5 * 2)); 
    GPIOB->MODER &= ~(3U << (6 * 2) | 3U << (7 * 2));
    GPIOB->MODER |= (2U << (6 * 2) | 2U << (7 * 2)); 
    GPIOB->OTYPER |= (1U << 6) | (1U << 7); 
    GPIOB->PUPDR |= (1U << (6 * 2)) | (1U << (7 * 2)); 
    GPIOB->AFR[0] |= (4U << (6 * 4)) | (4U << (7 * 4)); 
    
    uint32_t pclk1_freq = SystemCoreClock; 
    uint32_t freq_mhz = pclk1_freq / 1000000;
    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0;
    I2C1->CR2 = freq_mhz; 
    I2C1->CCR = pclk1_freq / 200000; 
    I2C1->TRISE = freq_mhz + 1; 
    I2C1->CR1 |= I2C_CR1_PE; 

    ADC1->CR2 |= ADC_CR2_ADON; 
    ADC123_COMMON->CCR = (ADC123_COMMON->CCR & ~ADC_CCR_ADCPRE) | ADC_CCR_ADCPRE_0; 
    ADC123_COMMON->CCR |= ADC_CCR_TSVREFE; 
    ADC1->SMPR1 = 0x07FFFFFF; 
    
    *SCB_DEMCR |= 0x01000000;
    *DWT_CYCCNT = 0;
    *DWT_CONTROL |= 1;
    
    for(volatile int j=0; j<10000; j++); 
    
}

uint32_t BareMetal_ADC_Read(uint8_t channel) {
    ADC1->SQR3 = channel;
    ADC1->CR2 |= ADC_CR2_SWSTART;
    uint32_t timeout = 100000;
    while (!(ADC1->SR & ADC_SR_EOC)) { if (--timeout == 0) return 0; }
    volatile uint32_t dummy = ADC1->DR;
    (void)dummy;
    
    for (volatile int i = 0; i < 50; i++);
    
    ADC1->CR2 |= ADC_CR2_SWSTART;
    timeout = 100000;
    while (!(ADC1->SR & ADC_SR_EOC)) { if (--timeout == 0) return 0; }
    return ADC1->DR;
}

void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    uint32_t pin_num = __builtin_ctz(GPIO_Pin);
    uint32_t temp = GPIOx->MODER;
    temp &= ~(3U << (pin_num * 2));
    temp |= (1U << (pin_num * 2)); 
    GPIOx->MODER = temp;
}

void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    uint32_t pin_num = __builtin_ctz(GPIO_Pin);
    GPIOx->MODER &= ~(3U << (pin_num * 2)); 
}

uint8_t DS18B20_Reset(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    uint8_t response = 0;
    Set_Pin_Output(GPIOx, GPIO_Pin);
    GPIOx->BSRR = (uint32_t)GPIO_Pin << 16; 
    Delay_us(480);
    Set_Pin_Input(GPIOx, GPIO_Pin); 
    Delay_us(80);
    if (!(GPIOx->IDR & GPIO_Pin)) response = 1; 
    Delay_us(400);
    return response;
}

void DS18B20_Write_Bit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t bit) {
    Set_Pin_Output(GPIOx, GPIO_Pin);
    GPIOx->BSRR = (uint32_t)GPIO_Pin << 16; 
    Delay_us(2);
    if (bit) {
        Set_Pin_Input(GPIOx, GPIO_Pin); 
        Delay_us(60);
    } else {
        Delay_us(60); 
        Set_Pin_Input(GPIOx, GPIO_Pin); 
    }
    Delay_us(2);
}

uint8_t DS18B20_Read_Bit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    uint8_t bit = 0;
    Set_Pin_Output(GPIOx, GPIO_Pin);
    GPIOx->BSRR = (uint32_t)GPIO_Pin << 16; 
    Delay_us(2);
    Set_Pin_Input(GPIOx, GPIO_Pin); 
    Delay_us(10);
    if (GPIOx->IDR & GPIO_Pin) bit = 1;
    Delay_us(50);
    return bit;
}

void DS18B20_Write_Byte(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t data) {
    for (int i = 0; i < 8; i++) {
        DS18B20_Write_Bit(GPIOx, GPIO_Pin, data & 0x01);
        data >>= 1;
    }
}

uint8_t DS18B20_Read_Byte(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (DS18B20_Read_Bit(GPIOx, GPIO_Pin)) {
            data |= (1 << i);
        }
    }
    return data;
}

void DS18B20_Start_Conversion(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    if (DS18B20_Reset(GPIOx, GPIO_Pin)) {
        DS18B20_Write_Byte(GPIOx, GPIO_Pin, 0xCC); 
        DS18B20_Write_Byte(GPIOx, GPIO_Pin, 0x44); 
    }
}

float DS18B20_Read_Temp(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    uint8_t temp_l, temp_h;
    if (DS18B20_Reset(GPIOx, GPIO_Pin)) {
        DS18B20_Write_Byte(GPIOx, GPIO_Pin, 0xCC); 
        DS18B20_Write_Byte(GPIOx, GPIO_Pin, 0xBE); 
        temp_l = DS18B20_Read_Byte(GPIOx, GPIO_Pin);
        temp_h = DS18B20_Read_Byte(GPIOx, GPIO_Pin);
        
        int16_t raw_temp = (temp_h << 8) | temp_l;
        return (float)raw_temp / 16.0f;
    }
    return -999.0f; 
}

uint8_t mpu6050_connected = 0;

uint8_t I2C_Start(void) {
    I2C1->CR1 |= I2C_CR1_START;
    uint32_t timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_SB)) { if (--timeout == 0) return 0; }
    return 1;
}
uint8_t I2C_Write(uint8_t data) {
    I2C1->DR = data;
    uint32_t timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_TXE)) { if (--timeout == 0) return 0; }
    return 1;
}
uint8_t I2C_Address(uint8_t addr) {
    I2C1->DR = addr;
    uint32_t timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_ADDR)) { if (--timeout == 0) return 0; }
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    return 1;
}
void I2C_Stop(void) {
    I2C1->CR1 |= I2C_CR1_STOP;
}

uint8_t MPU6050_WriteReg(uint8_t reg, uint8_t data) {
    if (!I2C_Start()) { I2C_Stop(); return 0; }
    if (!I2C_Address(0xD0)) { I2C_Stop(); return 0; }
    if (!I2C_Write(reg)) { I2C_Stop(); return 0; }
    if (!I2C_Write(data)) { I2C_Stop(); return 0; }
    I2C_Stop();
    return 1;
}

uint8_t MPU6050_ReadRegs(uint8_t reg, uint8_t* buffer, uint8_t size) {
    if (!I2C_Start()) { I2C_Stop(); return 0; }
    if (!I2C_Address(0xD0)) { I2C_Stop(); return 0; }
    if (!I2C_Write(reg)) { I2C_Stop(); return 0; }
    if (!I2C_Start()) { I2C_Stop(); return 0; }
    I2C1->DR = 0xD1;
    uint32_t timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_ADDR)) { if (--timeout == 0) { I2C_Stop(); return 0; } }
    
    if (size == 1) {
        I2C1->CR1 &= ~I2C_CR1_ACK;
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        I2C_Stop();
        timeout = 100000;
        while (!(I2C1->SR1 & I2C_SR1_RXNE)) { if (--timeout == 0) return 0; }
        buffer[0] = I2C1->DR;
    } else {
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        for (int i = 0; i < size; i++) {
            if (i == size - 1) {
                I2C1->CR1 &= ~I2C_CR1_ACK;
                I2C_Stop();
            } else {
                I2C1->CR1 |= I2C_CR1_ACK;
            }
            timeout = 100000;
            while (!(I2C1->SR1 & I2C_SR1_RXNE)) { if (--timeout == 0) return 0; }
            buffer[i] = I2C1->DR;
        }
    }
    return 1;
}

void MPU6050_Init(void) {
    mpu6050_connected = 1;
    if (!MPU6050_WriteReg(0x6B, 0x00)) mpu6050_connected = 0; 
    if (!MPU6050_WriteReg(0x1C, 0x10)) mpu6050_connected = 0; 
}

float MPU6050_Read_Vib(void) {
    if (!mpu6050_connected) return 0.0f;
    uint8_t data[6];
    if (!MPU6050_ReadRegs(0x3B, data, 6)) {
        mpu6050_connected = 0;
        return 0.0f;
    }
    int16_t accel_x = (data[0] << 8) | data[1];
    int16_t accel_y = (data[2] << 8) | data[3];
    int16_t accel_z = (data[4] << 8) | data[5];
    
    float x = accel_x / 4096.0f;
    float y = accel_y / 4096.0f;
    float z = accel_z / 4096.0f;
    
    float total_g = sqrtf(x*x + y*y + z*z);
    float vib = total_g - 1.0f; 
    if (vib < 0) vib = -vib;
    return vib;
}

// --- I2C 16x2 LCD Driver (PCF8574 Backpack) ---
#define LCD_ADDR_A  0x4E // 0x27 shifted left by 1 for write
#define LCD_ADDR_B  0x7E // 0x3F shifted left by 1 for write
static uint8_t lcd_i2c_addr = LCD_ADDR_A;
static uint8_t lcd_backlight_val = 0x08; // 0x08 = Backlight ON, 0x00 = OFF

static void LCD_I2C_Write(uint8_t addr, uint8_t data) {
    if (!I2C_Start()) { I2C_Stop(); return; }
    if (!I2C_Address(addr)) { I2C_Stop(); return; }
    I2C_Write(data);
    I2C_Stop();
}

static void LCD_SendNibble(uint8_t nibble, uint8_t rs_flag) {
    uint8_t base = (nibble & 0xF0) | lcd_backlight_val | rs_flag;
    LCD_I2C_Write(lcd_i2c_addr, base | 0x04); // EN = 1
    Delay_us(2);
    LCD_I2C_Write(lcd_i2c_addr, base & ~0x04); // EN = 0
    Delay_us(50);
}

static void LCD_Write(uint8_t data, uint8_t rs_flag) {
    LCD_SendNibble(data & 0xF0, rs_flag);
    LCD_SendNibble((data << 4) & 0xF0, rs_flag);
}

static void LCD_SendCommand(uint8_t cmd) {
    LCD_Write(cmd, 0);
}

static void LCD_SendData(uint8_t data) {
    LCD_Write(data, 0x01); // RS = 1
}

void LCD_Init(void) {
    // Auto-detect LCD I2C address
    lcd_i2c_addr = LCD_ADDR_A;
    if (!I2C_Start()) {
        I2C_Stop();
        lcd_i2c_addr = LCD_ADDR_B;
    } else {
        if (!I2C_Address(LCD_ADDR_A)) {
            lcd_i2c_addr = LCD_ADDR_B;
        }
        I2C_Stop();
    }
    
    HAL_Delay(50); // Wait for LCD power-up
    
    LCD_SendNibble(0x30, 0);
    HAL_Delay(5);
    LCD_SendNibble(0x30, 0);
    HAL_Delay(1);
    LCD_SendNibble(0x30, 0);
    HAL_Delay(1);
    LCD_SendNibble(0x20, 0); // set to 4-bit mode
    HAL_Delay(1);
    
    LCD_SendCommand(0x28); // 4-bit mode, 2 lines, 5x8 font
    LCD_SendCommand(0x0C); // Display ON, Cursor OFF
    LCD_SendCommand(0x06); // Increment cursor
    LCD_SendCommand(0x01); // Clear display
    HAL_Delay(2);
}

void LCD_SetCursor(uint8_t col, uint8_t row) {
    uint8_t addr = (row == 0) ? (0x00 + col) : (0x40 + col);
    LCD_SendCommand(0x80 | addr);
}

void LCD_PrintString(const char* str) {
    while (*str) {
        LCD_SendData(*str++);
    }
}

void LCD_Update(float c1, float c2, float c3, float c4) {
    uint8_t active_cells = 0;
    if (c1 > 0.5f) active_cells++;
    if (c2 > 0.5f) active_cells++;
    if (c3 > 0.5f) active_cells++;
    if (c4 > 0.5f) active_cells++;
    
    char line0[17] = "                ";
    char line1[17] = "                ";
    
    if (active_cells == 0) {
        memcpy(line0, "    NO CELL     ", 16);
        memcpy(line1, "   CONNECTED    ", 16);
    } else {
        char temp[32];
        
        if (c1 > 0.5f) {
            int i_val = (int)c1; int f_val = (int)((c1 - i_val)*100); if (f_val < 0) f_val = -f_val;
            sprintf(temp, "1:%d.%02d", i_val, f_val);
            memcpy(&line0[0], temp, strlen(temp));
        }
        
        if (c2 > 0.5f) {
            int i_val = (int)c2; int f_val = (int)((c2 - i_val)*100); if (f_val < 0) f_val = -f_val;
            sprintf(temp, "2:%d.%02d", i_val, f_val);
            memcpy(&line0[7], temp, strlen(temp));
        }
        
        sprintf(temp, "%dS", active_cells);
        memcpy(&line0[14], temp, strlen(temp));
        
        if (c3 > 0.5f) {
            int i_val = (int)c3; int f_val = (int)((c3 - i_val)*100); if (f_val < 0) f_val = -f_val;
            sprintf(temp, "3:%d.%02d", i_val, f_val);
            memcpy(&line1[0], temp, strlen(temp));
        }
        
        if (c4 > 0.5f) {
            int i_val = (int)c4; int f_val = (int)((c4 - i_val)*100); if (f_val < 0) f_val = -f_val;
            sprintf(temp, "4:%d.%02d", i_val, f_val);
            memcpy(&line1[7], temp, strlen(temp));
        }
    }
    
    line0[16] = '\0';
    line1[16] = '\0';
    
    LCD_SetCursor(0, 0);
    LCD_PrintString(line0);
    LCD_SetCursor(0, 1);
    LCD_PrintString(line1);
}

// --- USART3 Driver & Ring Buffer (ESP-01S) ---
#define ESP_RX_BUF_SIZE  256
static char esp_rx_buffer[ESP_RX_BUF_SIZE];
static volatile uint16_t esp_rx_head = 0;
static volatile uint16_t esp_rx_tail = 0;

void USART3_Init(void) {
    // Enable clocks for GPIOC and USART3
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    
    // Configure PC10 (TX) and PC11 (RX) to AF7 (USART3)
    GPIOC->MODER &= ~(3U << (10 * 2) | 3U << (11 * 2));
    GPIOC->MODER |= (2U << (10 * 2) | 2U << (11 * 2)); // Alternate function mode
    GPIOC->OSPEEDR |= (3U << (10 * 2) | 3U << (11 * 2)); // Very high speed
    GPIOC->PUPDR |= (1U << (11 * 2)); // Pull-up on RX pin
    
    // Map PC10 and PC11 to alternate function AF7 (USART3)
    GPIOC->AFR[1] &= ~(0xFU << ((10 - 8) * 4) | 0xFU << ((11 - 8) * 4));
    GPIOC->AFR[1] |= (7U << ((10 - 8) * 4) | 7U << ((11 - 8) * 4));
    
    // Configure Baud Rate: 115200 (16MHz clock -> BRR = 0x008B)
    USART3->BRR = 0x008B;
    
    // Enable Transmitter, Receiver, USART3 and RX Register Not Empty interrupt
    USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE;
    
    // Enable USART3 Interrupt in NVIC
    NVIC_EnableIRQ(USART3_IRQn);
}

void USART3_SendChar(char c) {
    while (!(USART3->SR & USART_SR_TXE));
    USART3->DR = c;
}

void USART3_SendString(const char* str) {
    while (*str) {
        USART3_SendChar(*str++);
    }
}

void USART3_IRQHandler(void) {
    if (USART3->SR & USART_SR_RXNE) {
        char c = USART3->DR;
        uint16_t next = (esp_rx_head + 1) % ESP_RX_BUF_SIZE;
        if (next != esp_rx_tail) {
            esp_rx_buffer[esp_rx_head] = c;
            esp_rx_head = next;
        }
    }
}

uint8_t ESP_WaitForResponse(const char* expected, uint32_t timeout_ms) {
    uint32_t start = HAL_GetTick();
    char match_buf[ESP_RX_BUF_SIZE];
    uint16_t idx = 0;
    
    while ((HAL_GetTick() - start) < timeout_ms) {
        if (esp_rx_tail != esp_rx_head) {
            char c = esp_rx_buffer[esp_rx_tail];
            esp_rx_tail = (esp_rx_tail + 1) % ESP_RX_BUF_SIZE;
            
            // Echo raw character to debug UART (USART2)
            UART_SendChar(c);
            
            // Skip null bytes: binary MQTT broker data (e.g. CONNACK \x00\x00)
            // can poison strstr() which stops searching at '\0'
            if (c == '\0') continue;
            
            match_buf[idx++] = c;
            match_buf[idx] = '\0';
            
            if (strstr(match_buf, expected) != NULL) {
                return 1;
            }
            
            if (idx >= ESP_RX_BUF_SIZE - 1) {
                memmove(match_buf, &match_buf[1], idx - 1);
                idx--;
            }
        }
    }
    return 0;
}

// --- Raw TCP MQTT 3.1.1 implementation (works on any ESP8266 firmware) ---
// Encodes MQTT variable-length remaining length field into buf, returns byte count
static uint8_t MQTT_EncodeLength(uint32_t length, uint8_t *buf) {
    uint8_t count = 0;
    do {
        buf[count] = (uint8_t)(length & 0x7F);
        length >>= 7;
        if (length > 0) buf[count] |= 0x80;
        count++;
    } while (length > 0 && count < 4);
    return count;
}

// Sends raw bytes over the open TCP connection via AT+CIPSEND
static uint8_t ESP_CIPSend(const uint8_t *data, uint16_t len) {
    char cmd[32];
    sprintf(cmd, "AT+CIPSEND=%u\r\n", (unsigned)len);
    USART3_SendString(cmd);
    if (!ESP_WaitForResponse(">", 2000)) return 0;
    for (uint16_t i = 0; i < len; i++) USART3_SendChar((char)data[i]);
    return ESP_WaitForResponse("SEND OK", 5000);
}

// Sends MQTT CONNECT and waits for both SEND OK and CONNACK (+IPD) in ONE scan.
// Critical: on fast networks the broker's CONNACK arrives BEFORE "SEND OK",
// so sequential waits miss it. A single loop detects whichever arrives first.
// Returns 1 if CONNACK was received within 10s, 0 on timeout.
static uint8_t MQTT_ConnectAndAwaitConnack(const char *client_id) {
    static uint8_t pkt[128];
    uint16_t cid_len = (uint16_t)strlen(client_id);
    uint32_t remaining = 10 + 2 + cid_len;
    uint16_t idx = 0;

    pkt[idx++] = 0x10;
    uint8_t lb[4]; uint8_t lc = MQTT_EncodeLength(remaining, lb);
    for (uint8_t i = 0; i < lc; i++) pkt[idx++] = lb[i];
    pkt[idx++] = 0x00; pkt[idx++] = 0x04;
    pkt[idx++] = 'M'; pkt[idx++] = 'Q'; pkt[idx++] = 'T'; pkt[idx++] = 'T';
    pkt[idx++] = 0x04;  // Protocol level: MQTT 3.1.1
    pkt[idx++] = 0x02;  // Connect flags: Clean Session
    pkt[idx++] = 0x00; pkt[idx++] = 0x3C; // Keep-alive 60s
    pkt[idx++] = (uint8_t)(cid_len >> 8);
    pkt[idx++] = (uint8_t)(cid_len & 0xFF);
    for (uint16_t i = 0; i < cid_len; i++) pkt[idx++] = (uint8_t)client_id[i];

    // Request data send
    char cmd[32];
    sprintf(cmd, "AT+CIPSEND=%u\r\n", (unsigned)idx);
    USART3_SendString(cmd);
    if (!ESP_WaitForResponse(">", 2000)) return 0;

    // Transmit MQTT CONNECT bytes
    for (uint16_t i = 0; i < idx; i++) USART3_SendChar((char)pkt[i]);

    // Wait up to 10s for BOTH "SEND OK" and CONNACK "+IPD" in a single scan.
    // We must not exit on SEND OK alone - CONNACK may not have arrived yet.
    // We must not scan sequentially - CONNACK may arrive before SEND OK.
    uint8_t got_send_ok = 0, got_connack = 0;
    static char wbuf[256];
    uint16_t widx = 0;
    wbuf[0] = '\0';
    uint32_t start = HAL_GetTick();

    while ((HAL_GetTick() - start) < 10000) {
        if (esp_rx_tail != esp_rx_head) {
            char c = esp_rx_buffer[esp_rx_tail];
            esp_rx_tail = (esp_rx_tail + 1) % ESP_RX_BUF_SIZE;
            UART_SendChar(c);
            if (c == '\0') continue; // skip null bytes from binary broker data
            wbuf[widx++] = c;
            wbuf[widx]   = '\0';
            if (!got_send_ok && strstr(wbuf, "SEND OK")) got_send_ok = 1;
            if (!got_connack && strstr(wbuf, "+IPD"))    got_connack = 1;
            // Once both are confirmed, stop immediately
            if (got_send_ok && got_connack) break;
            // Slide window to keep buffer from overflowing
            if (widx >= 255) { memmove(wbuf, &wbuf[1], widx - 1); widx--; }
        }
    }

    if (got_connack) {
        // Allow remaining CONNACK binary bytes to arrive then discard them
        // so null bytes don't bleed into the next AT+CIPSEND ">" search
        HAL_Delay(300);
        esp_rx_tail = esp_rx_head;
    }

    return got_connack;
}

// Builds and sends a MQTT 3.1.1 PUBLISH packet (QoS 0, no retain)
static uint8_t MQTT_SendPublish(const char *topic, const char *payload) {
    static uint8_t pkt[512];
    uint16_t tlen = (uint16_t)strlen(topic);
    uint16_t plen = (uint16_t)strlen(payload);
    uint32_t remaining = 2 + tlen + plen; // topic length field + topic + payload
    uint16_t idx = 0;

    pkt[idx++] = 0x30;                             // Fixed header: PUBLISH QoS 0
    uint8_t lb[4]; uint8_t lc = MQTT_EncodeLength(remaining, lb);
    for (uint8_t i = 0; i < lc; i++) pkt[idx++] = lb[i];
    pkt[idx++] = (uint8_t)(tlen >> 8);
    pkt[idx++] = (uint8_t)(tlen & 0xFF);
    for (uint16_t i = 0; i < tlen; i++) pkt[idx++] = (uint8_t)topic[i];
    for (uint16_t i = 0; i < plen; i++) pkt[idx++] = (uint8_t)payload[i];

    return ESP_CIPSend(pkt, idx);
}

void USART3_MQTT_Publish(const char* topic, const char* payload) {
    if (!wifi_connected || !mqtt_connected) return;
    if (!MQTT_SendPublish(topic, payload)) {
        // TCP connection likely dropped - mark disconnected for reconnect
        UART_SendString("[MQTT] Publish failed - TCP connection lost\r\n");
        mqtt_connected = 0;
    }
}

uint8_t ESP_ConnectWiFi(void) {

    UART_SendString("Testing ESP-01S UART...\r\n");

    // Wait for ESP-01S to finish booting (esp8266 takes ~1-2s after power-on).
    // During boot it emits garbage at 74880 baud that fills the ring buffer -
    // flushing after the wait discards all that before the real AT test.
    HAL_Delay(2000);
    esp_rx_tail = esp_rx_head; // flush boot garbage

    // Try AT up to 5 times (1s apart) to handle slow-starting modules
    uint8_t at_ok = 0;
    for (uint8_t at_try = 0; at_try < 5; at_try++) {
        USART3_SendString("AT\r\n");
        if (ESP_WaitForResponse("OK", 1500)) {
            at_ok = 1;
            break;
        }
        char retry_msg[32];
        sprintf(retry_msg, "AT retry %d/5...\r\n", at_try + 1);
        UART_SendString(retry_msg);
        HAL_Delay(500);
        esp_rx_tail = esp_rx_head; // flush between retries
    }

    if (!at_ok) {
        UART_SendString("ESP-01S not responding to AT! Check wiring/baud.\r\n");
        return 0;
    }
    
    // Set to Station mode
    USART3_SendString("AT+CWMODE=1\r\n");
    ESP_WaitForResponse("OK", 2000);
    
    // Connect to SSID
    UART_SendString("Connecting to WiFi AP: MONISH...\r\n");
    USART3_SendString("AT+CWJAP=\"MONISH\",\"12345678\"\r\n");
    if (ESP_WaitForResponse("WIFI GOT IP", 15000)) {
        UART_SendString("Connected to MONISH successfully!\r\n");
        wifi_connected = 1;
        
        // Allow 3s for IP stack to stabilise after WiFi join
        HAL_Delay(3000);

        // --- Raw TCP MQTT connection (bypasses AT+MQTT* firmware requirement) ---
        // Step 1: Ensure single-connection mode
        UART_SendString("[MQTT] Setting single TCP mode...\r\n");
        USART3_SendString("AT+CIPMUX=0\r\n");
        ESP_WaitForResponse("OK", 2000);

        // Step 2: Open TCP connection to HiveMQ public broker
        UART_SendString("[MQTT] Opening TCP to broker.hivemq.com:1883...\r\n");
        USART3_SendString("AT+CIPSTART=\"TCP\",\"broker.hivemq.com\",1883\r\n");
        if (ESP_WaitForResponse("CONNECT", 15000)) {
            UART_SendString("[MQTT] TCP connected. Sending MQTT CONNECT packet...\r\n");

            // Step 3: Send MQTT CONNECT and wait for CONNACK in one combined scan
            char client_id[32];
            sprintf(client_id, "stm32_%lu", HAL_GetTick());
            if (MQTT_ConnectAndAwaitConnack(client_id)) {
                UART_SendString("[MQTT] CONNACK received! Broker connected.\r\n");
                mqtt_connected = 1;
                USART3_MQTT_Publish("battery/terminal", "[SYSTEM] STM32 BMS Online. MQTT Connected.");
            } else {
                UART_SendString("[MQTT] No CONNACK from broker (timeout).\r\n");
                mqtt_connected = 0;
            }
        } else {
            UART_SendString("[MQTT] TCP connect to broker failed.\r\n");
            mqtt_connected = 0;
        }
        return 1;
    }

    UART_SendString("Failed to connect to MONISH!\r\n");
    wifi_connected = 0;
    mqtt_connected = 0;
    return 0;
}


void Measure_And_Print_Battery(float t1, float t2, float vib) {
    uint32_t raw_b1_sum = 0, raw_b2_sum = 0, raw_b3_sum = 0, raw_b4_sum = 0;
    uint32_t raw_vref_sum = 0;
    for(int i = 0; i < 64; i++) {
        raw_vref_sum += BareMetal_ADC_Read(17);
    }
    float true_3v3 = (1.21f * 4095.0f) / (raw_vref_sum / 64.0f);
    
    for(int i = 0; i < 256; i++) {
        raw_b1_sum += BareMetal_ADC_Read(11);
        raw_b2_sum += BareMetal_ADC_Read(12);
        raw_b3_sum += BareMetal_ADC_Read(13);
        raw_b4_sum += BareMetal_ADC_Read(14);
    }
    
    uint32_t raw_b1 = raw_b1_sum / 256;
    uint32_t raw_b2 = raw_b2_sum / 256;
    uint32_t raw_b3 = raw_b3_sum / 256;
    uint32_t raw_b4 = raw_b4_sum / 256;
    
    float pin_v1 = (raw_b1 * true_3v3) / 4095.0f;
    float pin_v2 = (raw_b2 * true_3v3) / 4095.0f;
    float pin_v3 = (raw_b3 * true_3v3) / 4095.0f;
    float pin_v4 = (raw_b4 * true_3v3) / 4095.0f;
    
    float tap_v1 = pin_v1 * SCALE_B1;
    float tap_v2 = pin_v2 * SCALE_B2;
    float tap_v3 = pin_v3 * SCALE_B3;
    float tap_v4 = pin_v4 * SCALE_B4;
    
    if(pin_v1 < 0.25f) tap_v1 = 0.0f;
    if(pin_v2 < 0.25f) tap_v2 = 0.0f;
    if(pin_v3 < 0.25f) tap_v3 = 0.0f;
    if(pin_v4 < 0.25f) tap_v4 = 0.0f;

    if (tap_v1 > 0.0f) {
        if (tap_v1 < 0.0f) tap_v1 = 0.0f;
    }
    if (tap_v2 > 0.0f) {
        if (tap_v2 < 0.0f) tap_v2 = 0.0f;
    }
    if (tap_v3 > 0.0f) {
        if (tap_v2 == 0.0f) {
            tap_v3 -= 0.14f; // 1S single cell test mode offset
        }
        if (tap_v3 < 0.0f) tap_v3 = 0.0f;
    }
    if (tap_v4 > 0.0f) {
        if (tap_v3 == 0.0f) {
            tap_v4 -= 0.18f; // 1S single cell test mode offset
        }
        if (tap_v4 < 0.0f) tap_v4 = 0.0f;
    }
    
    float cell1 = 0.0f;
    float cell2 = 0.0f;
    float cell3 = 0.0f;
    float cell4 = 0.0f;

    // Cell 1: Requires Tap 1 to be connected (>0.3V)
    if (tap_v1 > 0.3f) {
        cell1 = tap_v1;
    }

    // Cell 2: Requires both Tap 1 and Tap 2 to be connected
    if (tap_v1 > 0.3f && tap_v2 > 0.3f) {
        cell2 = tap_v2 - tap_v1;
    }

    // Cell 3: Requires both Tap 2 and Tap 3 to be connected
    if (tap_v2 > 0.3f && tap_v3 > 0.3f) {
        cell3 = tap_v3 - tap_v2;
    }

    // Cell 4: Requires both Tap 3 and Tap 4 to be connected
    if (tap_v3 > 0.3f && tap_v4 > 0.3f) {
        cell4 = tap_v4 - tap_v3;
    }

    
    if(cell1 < 0.0f) cell1 = 0.0f;
    if(cell2 < 0.0f) cell2 = 0.0f;
    if(cell3 < 0.0f) cell3 = 0.0f;
    if(cell4 < 0.0f) cell4 = 0.0f;
    
    float pack_voltage = cell1 + cell2 + cell3 + cell4;
    
    uint32_t raw_curr_sum = 0;
    for(int i = 0; i < 4096; i++) {
        raw_curr_sum += BareMetal_ADC_Read(15);
    }
    uint32_t raw_curr = raw_curr_sum / 4096;
    float pin_v_curr = (raw_curr * true_3v3) / 4095.0f;
    
    float current_A = (ACS712_ZERO_VOLTAGE - pin_v_curr) / ACS712_SENSITIVITY;
    
    if(pin_v_curr < 0.20f) { current_A = 0.0f; }
    if(current_A > -0.15f && current_A < 0.15f) current_A = 0.0f;
    
    // Calculate SoC based on the average of active cell voltages
    float total_active_voltage = 0.0f;
    uint8_t active_cells = 0;
    if (cell1 > 0.5f) { total_active_voltage += cell1; active_cells++; }
    if (cell2 > 0.5f) { total_active_voltage += cell2; active_cells++; }
    if (cell3 > 0.5f) { total_active_voltage += cell3; active_cells++; }
    if (cell4 > 0.5f) { total_active_voltage += cell4; active_cells++; }
    
    if (active_cells > 0) {
        float pack_ref_min = (float)active_cells * 3.70f;
        float pack_ref_max = (float)active_cells * 4.20f;
        current_soc = roundf(((pack_voltage - pack_ref_min) / (pack_ref_max - pack_ref_min)) * 100.0f);
        if (current_soc > 100.0f) current_soc = 100.0f;
        if (current_soc < 0.0f) current_soc = 0.0f;
    } else {
        current_soc = 0.0f;
    }

    // Determine state (Idle, Charging, Discharging) directly from the current direction and magnitude
    if (current_A >= 0.15f) {
        current_charge_status = 1; // Charging
    } else if (current_A <= -0.15f) {
        current_charge_status = 2; // Discharging
    } else {
        current_charge_status = 0; // Idle
    }
    
    // Apply sign convention: Charging is (+), Discharging is (-)
    if (current_charge_status == 1) {
        current_A = fabs(current_A);
    } else if (current_charge_status == 2) {
        current_A = -fabs(current_A);
    } else {
        current_A = 0.0f;
    }
    
    uint32_t raw_mq7_sum = 0;
    for(int i = 0; i < 256; i++) {
        raw_mq7_sum += BareMetal_ADC_Read(10);
    }
    float pin_v_mq7 = ((raw_mq7_sum / 256.0f) * true_3v3) / 4095.0f;
    float vout_mq7 = pin_v_mq7 * 2.0f;
    
    float Rs_clean = (4.90f - 0.45f) / 0.45f;
    float R0 = Rs_clean / 27.0f;
    
    float ppm = 0.0f;
    if (vout_mq7 > 0.30f) { 
        float Rs = (4.90f - vout_mq7) / vout_mq7;
        float ratio = Rs / R0;
        ppm = 100.0f * powf(ratio, -1.51f);
    } else {
        ppm = 0.0f; 
    }
    if (ppm < 0) ppm = 0;
    
    char msg[256];
    int p_i = (int)pack_voltage; int p_f = (int)((pack_voltage - p_i)*100); if (p_f < 0) p_f = -p_f;
    int c1_i = (int)cell1; int c1_f = (int)((cell1 - c1_i)*100);
    int c2_i = (int)cell2; int c2_f = (int)((cell2 - c2_i)*100);
    int c3_i = (int)cell3; int c3_f = (int)((cell3 - c3_i)*100);
    int c4_i = (int)cell4; int c4_f = (int)((cell4 - c4_i)*100);
    
    int curr_sign = (current_A < 0) ? -1 : 1;
    float abs_curr = current_A * curr_sign;
    int curr_i = (int)abs_curr; int curr_f = (int)((abs_curr - curr_i)*100);
    
    int pv1_i = (int)pin_v1; int pv1_f = (int)((pin_v1 - pv1_i)*1000);
    int pv2_i = (int)pin_v2; int pv2_f = (int)((pin_v2 - pv2_i)*1000);
    int pv3_i = (int)pin_v3; int pv3_f = (int)((pin_v3 - pv3_i)*1000);
    int pv4_i = (int)pin_v4; int pv4_f = (int)((pin_v4 - pv4_i)*1000);
    int pvc_i = (int)pin_v_curr; int pvc_f = (int)((pin_v_curr - pvc_i)*1000);
    int pvm_i = (int)pin_v_mq7; int pvm_f = (int)((pin_v_mq7 - pvm_i)*1000);
    
    char t1_str[16];
    char t2_str[16];
    if (t1 <= -99.0f) sprintf(t1_str, "ERR");
    else {
        int t1_i = (int)t1; int t1_f = (int)((t1 - t1_i)*10); if(t1_f < 0) t1_f = -t1_f;
        sprintf(t1_str, "%d.%dC", t1_i, t1_f);
    }
    if (t2 <= -99.0f) sprintf(t2_str, "ERR");
    else {
        int t2_i = (int)t2; int t2_f = (int)((t2 - t2_i)*10); if(t2_f < 0) t2_f = -t2_f;
        sprintf(t2_str, "%d.%dC", t2_i, t2_f);
    }
    
    char vib_str[16];
    int vib_i = (int)vib; int vib_f = (int)((vib - vib_i)*100); if(vib_f < 0) vib_f = -vib_f;
    sprintf(vib_str, "%d.%02dG", vib_i, vib_f);
    
    int ppm_i = (int)ppm; int ppm_f = (int)((ppm - ppm_i)*100); if(ppm_f < 0) ppm_f = -ppm_f;
    
    sprintf(msg, "\r\n[DEBUG] RAW ADC: %lu, %lu, %lu, %lu, CUR:%lu | PIN: %d.%03dV, %d.%03dV, %d.%03dV, %d.%03dV, CUR:%d.%03dV, MQ7:%d.%03dV\r\n", 
            (unsigned long)raw_b1, (unsigned long)raw_b2, (unsigned long)raw_b3, (unsigned long)raw_b4, (unsigned long)raw_curr,
            pv1_i, pv1_f, pv2_i, pv2_f, pv3_i, pv3_f, pv4_i, pv4_f, pvc_i, pvc_f, pvm_i, pvm_f);
    UART_SendString(msg);
    
    const char* curr_sign_str = " ";
    if (current_A < -0.15f) curr_sign_str = "-";
    else if (current_A > 0.15f) curr_sign_str = "+";

    const char* status_str = "IDLE";
    if (current_charge_status == 1) {
        if (current_A >= 2.0f) status_str = "FAST CHARGE";
        else status_str = "NORM CHARGE";
    } else if (current_charge_status == 2) {
        status_str = "DISCHARGING";
    }

    char time_str[32] = "N/A";
    if (current_charge_status == 1 && current_A > 0.05f) {
        float remaining_ah = 10.0f * (1.0f - (current_soc / 100.0f));
        float hours = remaining_ah / current_A;
        int time_hours = (int)hours;
        int time_mins = (int)((hours - time_hours) * 60.0f);
        if (time_hours > 99) sprintf(time_str, ">99h to full");
        else sprintf(time_str, "%dh %02dm to full", time_hours, time_mins);
    } else if (current_charge_status == 2 && current_A < -0.05f) {
        float remaining_ah = 10.0f * (current_soc / 100.0f);
        float hours = remaining_ah / (-current_A);
        int time_hours = (int)hours;
        int time_mins = (int)((hours - time_hours) * 60.0f);
        if (time_hours > 99) sprintf(time_str, ">99h to empty");
        else sprintf(time_str, "%dh %02dm to empty", time_hours, time_mins);
    }

    sprintf(msg, "[CALC ] Pack: %d.%02dV | Curr: %s%d.%02dA | SoC: %d%% | Status: %s | Time: %s | T1: %s | T2: %s | VIB: %s | CO: %d.%02dPPM | C1: %d.%02dV | C2: %d.%02dV | C3: %d.%02dV | C4: %d.%02dV\r\n", 
            p_i, p_f, curr_sign_str, curr_i, curr_f, (int)current_soc, status_str, time_str, t1_str, t2_str, vib_str, ppm_i, ppm_f, c1_i, c1_f, c2_i, c2_f, c3_i, c3_f, c4_i, c4_f);
            
    UART_SendString(msg);

    current_pack_v = pack_voltage;
    current_pack_a = current_A;
    current_t1 = t1;
    current_t2 = t2;
    current_max_vib = vib;
    current_ppm = ppm;
    current_c1 = cell1;
    current_c2 = cell2;
    current_c3 = cell3;
    current_c4 = cell4;
    pin_c1 = pin_v1;
    pin_c2 = pin_v2;
    pin_c3 = pin_v3;
    pin_c4 = pin_v4;
    pin_curr = pin_v_curr;
    pin_mq7 = pin_v_mq7;

    if ((t1 > 40.0f && t1 < 100.0f) || (t2 > 40.0f && t2 < 100.0f)) {
        COOLING_RELAY_ON();
    } else if ((t1 <= 35.0f || t1 >= 100.0f) && (t2 <= 35.0f || t2 >= 100.0f)) {
        COOLING_RELAY_OFF();
    }

    if (ml_isolation_signal == 1) {
        ISOLATION_RELAY_ON();
    } else {
        ISOLATION_RELAY_OFF();
    }

    switch (current_page) {
        case 0: ILI9341_DrawPage0(current_pack_v, current_pack_a, current_t1, current_t2, current_max_vib, current_ppm, current_soc, current_charge_status, wifi_connected); break;
        case 1: ILI9341_DrawPage1(current_c1, current_c2, current_c3, current_c4); break;
        case 2: ILI9341_DrawPage2(pin_c1, pin_c2, pin_c3, pin_c4, pin_curr, pin_mq7, current_max_vib, current_ppm); break;
    }

    // Update 16x2 I2C LCD with cell telemetry
    LCD_Update(cell1, cell2, cell3, cell4);

    // Periodically publish telemetry and terminal logs to MQTT
    static uint32_t last_pub_tick = 0;
    uint32_t current_tick = HAL_GetTick();
    if (mqtt_connected && (current_tick - last_pub_tick >= 3000)) {
        last_pub_tick = current_tick;
        
        char json[256];
        int c1_i = (int)cell1; int c1_f = (int)((cell1 - c1_i)*100);
        if (c1_f < 0) c1_f = -c1_f;
        int c2_i = (int)cell2; int c2_f = (int)((cell2 - c2_i)*100);
        if (c2_f < 0) c2_f = -c2_f;
        int c3_i = (int)cell3; int c3_f = (int)((cell3 - c3_i)*100);
        if (c3_f < 0) c3_f = -c3_f;
        int c4_i = (int)cell4; int c4_f = (int)((cell4 - c4_i)*100);
        if (c4_f < 0) c4_f = -c4_f;
        
        int curr_sign = (current_A < 0) ? -1 : 1;
        float abs_curr = current_A * curr_sign;
        int curr_i = (int)abs_curr; int curr_f = (int)((abs_curr - curr_i)*100);
        if (curr_f < 0) curr_f = -curr_f;
        const char* curr_sgn = (current_A < -0.15f) ? "-" : "";
        
        float max_t = (t1 > t2) ? t1 : t2;
        int temp_i = (int)max_t; int temp_f = (int)((max_t - temp_i)*10);
        if (temp_f < 0) temp_f = -temp_f;
        
        int gas_i = (int)ppm; int gas_f = (int)((ppm - gas_i)*100);
        if (gas_f < 0) gas_f = -gas_f;
        
        const char* relay_status = (ml_isolation_signal == 1) ? "CONNECTED" : "DISCONNECTED";
        
        // Classify system state for web DB status field
        const char* sys_status = "Healthy";
        if ((max_t > 50.0f && max_t < 100.0f) || ppm > 350.0f) sys_status = "Critical";
        else if ((max_t > 40.0f && max_t < 100.0f) || ppm > 150.0f) sys_status = "Warning";
        
        sprintf(json, "{\"cell1\":%d.%02d,\"cell2\":%d.%02d,\"cell3\":%d.%02d,\"cell4\":%d.%02d,\"current\":%s%d.%02d,\"temperature\":%d.%d,\"gas\":%d.%02d,\"status\":\"%s\",\"relay\":\"%s\"}",
                c1_i, c1_f, c2_i, c2_f, c3_i, c3_f, c4_i, c4_f,
                curr_sgn, curr_i, curr_f,
                temp_i, temp_f,
                gas_i, gas_f,
                sys_status, relay_status);
                
        USART3_MQTT_Publish("battery/live", json);
        
        // Form the exact same terminal log string as USART2 and publish
        char term_log[256];
        sprintf(term_log, "[CALC ] Pack: %d.%02dV | Curr: %s%d.%02dA | SoC: %d%% | Status: %s | T1: %s | T2: %s | VIB: %s | CO: %d.%02dPPM | C1: %d.%02dV | C2: %d.%02dV | C3: %d.%02dV | C4: %d.%02dV", 
                p_i, p_f, curr_sign_str, curr_i, curr_f, (int)current_soc, status_str, t1_str, t2_str, vib_str, ppm_i, ppm_f, c1_i, c1_f, c2_i, c2_f, c3_i, c3_f, c4_i, c4_f);
        USART3_MQTT_Publish("battery/terminal", term_log);
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  MX_USART2_UART_Init();
  printf("Starting Init...\r\n");
  
  printf("Hardware Init...\r\n");
  BareMetal_Hardware_Init();
  printf("Hardware Init OK\r\n");
  
  printf("MPU6050 Init...\r\n");
  MPU6050_Init();
  printf("MPU6050 Init OK\r\n");
  
  printf("ILI9341 Init...\r\n");
  ILI9341_Init();
  printf("ILI9341 Init OK\r\n");
  
  printf("16x2 I2C LCD Init...\r\n");
  LCD_Init();
  printf("16x2 I2C LCD Init OK\r\n");
  
  printf("USART3 Init (ESP-01S)...\r\n");
  USART3_Init();
  printf("USART3 Init OK\r\n");
  
  printf("ESP-01S Connecting WiFi...\r\n");
  ESP_ConnectWiFi();
  
  extern const uint16_t boot_logo[];
  ILI9341_DrawLogo(boot_logo);
  
  // Energize the Isolation Relay to establish runtime current draw and ground state before calibration
  ISOLATION_RELAY_ON();
  
  HAL_Delay(2000);
  
  UART_SendString("Bare-Metal Battery Monitor Initialized\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  DS18B20_Start_Conversion(GPIOB, GPIO_PIN_8);
  DS18B20_Start_Conversion(GPIOB, GPIO_PIN_9);
  
  while (1)
  {
      uint32_t start_time = HAL_GetTick();
      float max_vib = 0.0f;
      
      while ((HAL_GetTick() - start_time) < 750)
      {
          float current_vib = MPU6050_Read_Vib();
          if (current_vib > max_vib) {
              max_vib = current_vib;
          }
          
          static uint8_t last_btn_state = 1;
          uint8_t btn13 = (GPIOC->IDR & (1U << 13)) ? 1 : 0;
          uint8_t btn10 = (GPIOA->IDR & (1U << 10)) ? 1 : 0;
          uint8_t current_btn_state = (btn13 == 0 || btn10 == 0) ? 0 : 1; 
          
          if (current_btn_state == 0 && last_btn_state == 1) { 
              current_page = (current_page + 1) % 3;
              page_changed = 1;
          }
          last_btn_state = current_btn_state;
          
          if (page_changed) {
              ILI9341_WipeTransition(TFT_VIBRANT_BLUE);
              switch (current_page) {
                  case 0:
                      ILI9341_DrawHeader("BMS OVERVIEW", 0);
                      ILI9341_DrawPage0(current_pack_v, current_pack_a, current_t1, current_t2, current_max_vib, current_ppm, current_soc, current_charge_status, wifi_connected);
                      break;
                  case 1:
                      ILI9341_DrawHeader("CELL MONITOR", 1);
                      ILI9341_DrawPage1(current_c1, current_c2, current_c3, current_c4);
                      break;
                  case 2:
                      ILI9341_DrawHeader("SYS DIAGNOSTICS", 2);
                      ILI9341_DrawPage2(pin_c1, pin_c2, pin_c3, pin_c4, pin_curr, pin_mq7, current_max_vib, current_ppm);
                      break;
              }
              page_changed = 0;
          }
          
          HAL_Delay(1); 
      }
      
      float t1 = DS18B20_Read_Temp(GPIOB, GPIO_PIN_8);
      float t2 = DS18B20_Read_Temp(GPIOB, GPIO_PIN_9);
      
      DS18B20_Start_Conversion(GPIOB, GPIO_PIN_8);
      DS18B20_Start_Conversion(GPIOB, GPIO_PIN_9);
      
      Measure_And_Print_Battery(t1, t2, max_vib);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(huart->Instance==USART2)
  {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}

int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
