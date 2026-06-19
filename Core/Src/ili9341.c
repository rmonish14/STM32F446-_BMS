#include "ili9341.h"
#include <stdio.h>
#include <string.h>

// Standard 5x7 ASCII Font Table (covers characters 32 (space) to 126 (~))
static const uint8_t font5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // (space)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x00, 0x7F, 0x41, 0x41}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // \ (Backslash)
    {0x41, 0x41, 0x7F, 0x00, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x08, 0x14, 0x54, 0x54, 0x3C}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x00, 0x7F, 0x10, 0x28, 0x44}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x14, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x01, 0x02}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x08, 0x36, 0x41, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x41, 0x36, 0x08}, // }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}, // ~
};

// Static inline helper to pump a byte out of SPI1
static inline void SPI1_WriteByte(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE));
    *(volatile uint8_t *)&SPI1->DR = data;
}

// Static inline helper to wait for SPI1 to become idle
static inline void SPI1_Wait(void) {
    while (SPI1->SR & SPI_SR_BSY);
}

// Initialize SPI1 and GPIO Pins for ILI9341 display
void ILI9341_SPI_Init(void) {
    // 1. Enable GPIOA peripheral clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    
    // 2. Enable SPI1 peripheral clock
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    
    // 3. Configure PA5 (SCK) and PA7 (MOSI) as Alternate Function (AF5)
    GPIOA->MODER &= ~(3U << (5 * 2) | 3U << (7 * 2));
    GPIOA->MODER |= (2U << (5 * 2) | 2U << (7 * 2)); // Alternate Function
    
    GPIOA->OSPEEDR |= (3U << (5 * 2) | 3U << (7 * 2)); // Very high speed
    
    // Set alternate function AF5 for SPI1 on PA5 and PA7
    GPIOA->AFR[0] &= ~(0xFU << (5 * 4) | 0xFU << (7 * 4));
    GPIOA->AFR[0] |= (5U << (5 * 4) | 5U << (7 * 4)); // AF5
    
    // 4. Configure Control Pins: PA4 (CS), PA6 (DC), PA1 (RST) as general-purpose output push-pull
    GPIOA->MODER &= ~(3U << (4 * 2) | 3U << (6 * 2) | 3U << (1 * 2));
    GPIOA->MODER |= (1U << (4 * 2) | 1U << (6 * 2) | 1U << (1 * 2)); // Output mode
    
    GPIOA->OSPEEDR |= (3U << (4 * 2) | 3U << (6 * 2) | 3U << (1 * 2)); // Very high speed
    
    // Set default HIGH states (CS, DC, RST active low / idle high)
    TFT_CS_H();
    TFT_DC_H();
    TFT_RST_H();
    
    // 5. Configure SPI1 control register
    // Master mode, MSB first, CPOL=0, CPHA=0, Clock prescaler = APB2 / 4 (21MHz)
    // Software Slave Management (SSM=1, SSI=1)
    SPI1->CR1 = 0; // Clear CR1
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (1U << 3); // Prescaler fPCLK/4
    
    // Enable SPI1
    SPI1->CR1 |= SPI_CR1_SPE;
}

// Perform hard reset on ILI9341 display
static void ILI9341_Reset(void) {
    TFT_RST_H();
    for (volatile int i = 0; i < 50000; i++);
    TFT_RST_L();
    for (volatile int i = 0; i < 50000; i++);
    TFT_RST_H();
    for (volatile int i = 0; i < 150000; i++); // Wait for controller to wake up
}

// Send command byte to ILI9341
void ILI9341_WriteCommand(uint8_t cmd) {
    TFT_DC_L(); // Command mode
    TFT_CS_L(); // Select display
    SPI1_WriteByte(cmd);
    SPI1_Wait();
    TFT_CS_H(); // Release display
}

// Send data byte to ILI9341
void ILI9341_WriteData(uint8_t data) {
    TFT_DC_H(); // Data mode
    TFT_CS_L(); // Select display
    SPI1_WriteByte(data);
    SPI1_Wait();
    TFT_CS_H(); // Release display
}

// Configure drawing window bounds
void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    ILI9341_WriteCommand(0x2A); // Column Address Set
    ILI9341_WriteData(x0 >> 8);
    ILI9341_WriteData(x0 & 0xFF);
    ILI9341_WriteData(x1 >> 8);
    ILI9341_WriteData(x1 & 0xFF);
    
    ILI9341_WriteCommand(0x2B); // Page Address Set
    ILI9341_WriteData(y0 >> 8);
    ILI9341_WriteData(y0 & 0xFF);
    ILI9341_WriteData(y1 >> 8);
    ILI9341_WriteData(y1 & 0xFF);
    
    ILI9341_WriteCommand(0x2C); // Memory Write Command
}

// Run the full ILI9341 register initialization sequence
void ILI9341_Init(void) {
    ILI9341_SPI_Init();
    ILI9341_Reset();
    
    ILI9341_WriteCommand(0x01); // Software Reset
    for (volatile int i = 0; i < 50000; i++);
    
    // Power Control A
    ILI9341_WriteCommand(0xCB);
    ILI9341_WriteData(0x39);
    ILI9341_WriteData(0x2C);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x34);
    ILI9341_WriteData(0x02);
    
    // Power Control B
    ILI9341_WriteCommand(0xCF);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0xC1);
    ILI9341_WriteData(0x30);
    
    // Driver timing control A
    ILI9341_WriteCommand(0xE8);
    ILI9341_WriteData(0x85);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x78);
    
    // Driver timing control B
    ILI9341_WriteCommand(0xEA);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x00);
    
    // Power on sequence control
    ILI9341_WriteCommand(0xED);
    ILI9341_WriteData(0x64);
    ILI9341_WriteData(0x03);
    ILI9341_WriteData(0x12);
    ILI9341_WriteData(0x81);
    
    // Pump ratio control
    ILI9341_WriteCommand(0xF7);
    ILI9341_WriteData(0x20);
    
    // Power Control 1
    ILI9341_WriteCommand(0xC0);
    ILI9341_WriteData(0x23);
    
    // Power Control 2
    ILI9341_WriteCommand(0xC1);
    ILI9341_WriteData(0x10);
    
    // VCOM Control 1
    ILI9341_WriteCommand(0xC5);
    ILI9341_WriteData(0x3E);
    ILI9341_WriteData(0x28);
    
    // VCOM Control 2
    ILI9341_WriteCommand(0xC7);
    ILI9341_WriteData(0x86);
    
    // Memory Access Control (Orientation)
    ILI9341_WriteCommand(0x36);
    ILI9341_WriteData(TFT_MADCTL_VAL);
    
    // Pixel Format Set
    ILI9341_WriteCommand(0x3A);
    ILI9341_WriteData(0x55); // 16-bit RGB565
    
    // Frame Rate Control
    ILI9341_WriteCommand(0xB1);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x18); // 79Hz
    
    // Display Function Control
    ILI9341_WriteCommand(0xB6);
    ILI9341_WriteData(0x08);
    ILI9341_WriteData(0x82);
    ILI9341_WriteData(0x27);
    
    // 3Gamma Function Disable
    ILI9341_WriteCommand(0xF2);
    ILI9341_WriteData(0x00);
    
    // Gamma Curve Selected
    ILI9341_WriteCommand(0x26);
    ILI9341_WriteData(0x01);
    
    // Positive Gamma Correction
    ILI9341_WriteCommand(0xE0);
    uint8_t posGamma[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    for (int i = 0; i < 15; i++) ILI9341_WriteData(posGamma[i]);
    
    // Negative Gamma Correction
    ILI9341_WriteCommand(0xE1);
    uint8_t negGamma[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    for (int i = 0; i < 15; i++) ILI9341_WriteData(negGamma[i]);
    
    // Exit Sleep
    ILI9341_WriteCommand(0x11);
    for (volatile int i = 0; i < 150000; i++);
    
    // Display ON
    ILI9341_WriteCommand(0x29);
    
    // Initialize full display to clean black
    ILI9341_FillScreen(TFT_BLACK);
}

// Draw a single solid-color pixel
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT) return;
    ILI9341_SetAddressWindow(x, y, x, y);
    TFT_DC_H();
    TFT_CS_L();
    SPI1_WriteByte(color >> 8);
    SPI1_WriteByte(color & 0xFF);
    SPI1_Wait();
    TFT_CS_H();
}

// Draw a filled solid-color rectangle
void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if ((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT)) return;
    if ((x + w - 1) >= ILI9341_WIDTH) w = ILI9341_WIDTH - x;
    if ((y + h - 1) >= ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;
    
    ILI9341_SetAddressWindow(x, y, x + w - 1, y + h - 1);
    
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    
    TFT_DC_H();
    TFT_CS_L();
    
    uint32_t total_pixels = (uint32_t)w * h;
    for (uint32_t i = 0; i < total_pixels; i++) {
        // High speed register pumping: write directly to SPI1->DR
        while (!(SPI1->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&SPI1->DR = hi;
        while (!(SPI1->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&SPI1->DR = lo;
    }
    
    SPI1_Wait();
    TFT_CS_H();
}

// Draw full screen solid color
void ILI9341_FillScreen(uint16_t color) {
    ILI9341_FillRect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

// Draw a hollow rectangle border
void ILI9341_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    ILI9341_FillRect(x, y, w, 1, color);           // Top edge
    ILI9341_FillRect(x, y + h - 1, w, 1, color);   // Bottom edge
    ILI9341_FillRect(x, y, 1, h, color);           // Left edge
    ILI9341_FillRect(x + w - 1, y, 1, h, color);   // Right edge
}

// Highly optimized character rendering (sets one address window and bursts all pixels)
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if (c < 32 || c > 127) c = ' ';
    uint8_t idx = c - 32;
    
    uint16_t w = 6 * size;
    uint16_t h = 8 * size;
    ILI9341_SetAddressWindow(x, y, x + w - 1, y + h - 1);
    
    TFT_DC_H();
    TFT_CS_L();
    
    for (int8_t j = 0; j < 8; j++) { // Loop rows
        for (int8_t sy = 0; sy < size; sy++) { // Scale height
            for (int8_t i = 0; i < 6; i++) { // Loop columns (5 data + 1 space)
                uint8_t pixel_on = 0;
                if (i < 5) {
                    uint8_t line = font5x7[idx][i];
                    if (line & (1 << j)) {
                        pixel_on = 1;
                    }
                }
                uint16_t cur_color = pixel_on ? color : bg;
                uint8_t ch = cur_color >> 8;
                uint8_t cl = cur_color & 0xFF;
                for (int8_t sx = 0; sx < size; sx++) { // Scale width
                    while (!(SPI1->SR & SPI_SR_TXE));
                    *(volatile uint8_t *)&SPI1->DR = ch;
                    while (!(SPI1->SR & SPI_SR_TXE));
                    *(volatile uint8_t *)&SPI1->DR = cl;
                }
            }
        }
    }
    SPI1_Wait();
    TFT_CS_H();
}

// Draw a string using standard or scaled font sizes
void ILI9341_DrawString(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size) {
    while (*str) {
        ILI9341_DrawChar(x, y, *str++, color, bg, size);
        x += 6 * size;
        if (x + 6 * size >= ILI9341_WIDTH) {
            x = 0;
            y += 8 * size;
            if (y + 8 * size >= ILI9341_HEIGHT) {
                break;
            }
        }
    }
}

// Draw a horizontal progress bar
void ILI9341_DrawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t percentage, uint16_t bar_color, uint16_t bg_color) {
    if (percentage > 100) percentage = 100;
    uint16_t filled_width = (uint32_t)w * percentage / 100;
    
    // Draw filled portion
    if (filled_width > 0) {
        ILI9341_FillRect(x, y, filled_width, h, bar_color);
    }
    // Draw remaining background portion
    if (filled_width < w) {
        ILI9341_FillRect(x + filled_width, y, w - filled_width, h, bg_color);
    }
    // Draw grid border around the progress bar
    ILI9341_DrawRect(x - 1, y - 1, w + 2, h + 2, TFT_GRID_GREY);
}

// Draw static page layout header
void ILI9341_DrawHeader(const char* title, uint8_t page_num) {
    // Fill the header background area
    ILI9341_FillRect(0, 0, ILI9341_WIDTH, 28, TFT_DARK_GREY);
    // Draw Title
    ILI9341_DrawString(10, 6, title, TFT_VIBRANT_BLUE, TFT_DARK_GREY, 2);
    // Draw Page Indicator
    char page_str[10];
    sprintf(page_str, "P%d/3", page_num + 1);
    ILI9341_DrawString(275, 6, page_str, TFT_LIGHT_GREY, TFT_DARK_GREY, 2);
    // Separator line
    ILI9341_FillRect(0, 28, ILI9341_WIDTH, 2, TFT_GRID_GREY);
}

// --- Page 0: Main telemetry dashboard ---
void ILI9341_DrawPage0(float pack_v, float current, float t1, float t2, float vib, float ppm, float soc, uint8_t charge_status, uint8_t wifi_status) {
    char val_str[32];
    
    // Draw grid boundaries (Only once when entering, but here we paint over dynamic text areas)
    ILI9341_FillRect(160, 30, 2, 210, TFT_GRID_GREY); // Vertical split
    ILI9341_FillRect(0, 135, 320, 2, TFT_GRID_GREY);  // Horizontal split
    
    // --- TOP LEFT: Pack Voltage ---
    ILI9341_DrawString(10, 38, "PACK VOLTAGE", TFT_LIGHT_GREY, TFT_BLACK, 1);
    
    int v_i = (int)pack_v;
    int v_f = (int)((pack_v - v_i) * 100);
    if (v_f < 0) v_f = -v_f;
    sprintf(val_str, "%2d.%02dV", v_i, v_f);
    
    uint16_t v_color = TFT_NEON_GREEN;
    if (pack_v < 14.8f) v_color = TFT_NEON_RED;
    else if (pack_v < 15.4f) v_color = TFT_AMBER;
    
    ILI9341_DrawString(10, 56, val_str, v_color, TFT_BLACK, 3);
    
    // --- SoC and Charging Status ---
    sprintf(val_str, "SoC: %3d%%", (int)soc);
    uint16_t soc_color = TFT_NEON_GREEN;
    if (soc < 20.0f) soc_color = TFT_NEON_RED;
    else if (soc < 50.0f) soc_color = TFT_AMBER;
    
    ILI9341_DrawString(10, 88, val_str, soc_color, TFT_BLACK, 2);
    ILI9341_DrawProgressBar(10, 108, 140, 8, (uint8_t)soc, soc_color, TFT_DARK_GREY);
    
    const char* status_label = "STATUS: IDLE       ";
    uint16_t status_color = TFT_LIGHT_GREY;
    if (charge_status == 1) {
        if (current >= 2.0f) {
            status_label = "STATUS: FAST CHARGE";
        } else {
            status_label = "STATUS: NORM CHARGE";
        }
        status_color = TFT_NEON_GREEN;
    } else if (charge_status == 2) {
        status_label = "STATUS: DISCHARGING";
        status_color = TFT_AMBER;
    }
    ILI9341_DrawString(10, 122, status_label, status_color, TFT_BLACK, 1);
    
    // Draw WiFi Status
    const char* wifi_label = "WIFI: DISCONNECTED";
    uint16_t wifi_color = TFT_AMBER;
    if (wifi_status == 1) {
        wifi_label = "WIFI: CONNECTED   ";
        wifi_color = TFT_NEON_GREEN;
    }
    ILI9341_DrawString(172, 122, wifi_label, wifi_color, TFT_BLACK, 1);
    
    // --- BOTTOM LEFT: Current & Time ---
    ILI9341_DrawString(10, 143, "PACK CURRENT", TFT_LIGHT_GREY, TFT_BLACK, 1);
    
    int curr_sign = (current < 0) ? -1 : 1;
    float abs_curr = current * curr_sign;
    int c_i = (int)abs_curr;
    int c_f = (int)((abs_curr - c_i) * 100);
    if (c_f < 0) c_f = -c_f;
    sprintf(val_str, "%s%d.%02dA", (current < -0.15f ? "-" : (current > 0.15f ? "+" : " ")), c_i, c_f);
    
    uint16_t c_color = TFT_VIBRANT_TEAL;
    if (current < -0.15f) c_color = TFT_NEON_RED;
    else if (current > 0.15f) c_color = TFT_NEON_GREEN;
    
    ILI9341_DrawString(10, 161, val_str, c_color, TFT_BLACK, 3);

    // Time to Full/Empty
    char time_str[32] = "N/A";
    const char* time_label = "TIME REMAINING";
    
    if (charge_status == 1 && current > 0.05f) {
        time_label = "TIME TO FULL";
        float remaining_ah = 10.0f * (1.0f - (soc / 100.0f));
        float hours = remaining_ah / current;
        int time_hours = (int)hours;
        int time_mins = (int)((hours - time_hours) * 60.0f);
        if (time_hours > 99) {
            sprintf(time_str, ">99h");
        } else {
            sprintf(time_str, "%dh %02dm", time_hours, time_mins);
        }
    } else if (charge_status == 2 && current < -0.05f) {
        time_label = "TIME TO EMPTY";
        float remaining_ah = 10.0f * (soc / 100.0f);
        float hours = remaining_ah / (-current);
        int time_hours = (int)hours;
        int time_mins = (int)((hours - time_hours) * 60.0f);
        if (time_hours > 99) {
            sprintf(time_str, ">99h");
        } else {
            sprintf(time_str, "%dh %02dm", time_hours, time_mins);
        }
    } else {
        time_label = "TIME REMAINING";
        sprintf(time_str, "N/A");
    }
    
    ILI9341_FillRect(10, 195, 140, 32, TFT_BLACK);
    ILI9341_DrawString(10, 195, time_label, TFT_LIGHT_GREY, TFT_BLACK, 1);
    ILI9341_DrawString(10, 209, time_str, TFT_WHITE, TFT_BLACK, 2);
    
    // --- TOP RIGHT: Temperatures & Vibration ---
    ILI9341_DrawString(172, 38, "TEMPERATURES", TFT_LIGHT_GREY, TFT_BLACK, 1);
    
    // Temp 1
    if (t1 <= -99.0f) {
        sprintf(val_str, "T1: ERR   ");
    } else {
        int t1_i = (int)t1; int t1_f = (int)((t1 - t1_i) * 10); if (t1_f < 0) t1_f = -t1_f;
        sprintf(val_str, "T1: %2d.%dC", t1_i, t1_f);
    }
    uint16_t t1_color = (t1 > 45.0f || t1 < 0.0f) ? TFT_NEON_RED : TFT_VIBRANT_TEAL;
    ILI9341_DrawString(172, 53, val_str, t1_color, TFT_BLACK, 2);
    
    // Temp 2
    if (t2 <= -99.0f) {
        sprintf(val_str, "T2: ERR   ");
    } else {
        int t2_i = (int)t2; int t2_f = (int)((t2 - t2_i) * 10); if (t2_f < 0) t2_f = -t2_f;
        sprintf(val_str, "T2: %2d.%dC", t2_i, t2_f);
    }
    uint16_t t2_color = (t2 > 45.0f || t2 < 0.0f) ? TFT_NEON_RED : TFT_VIBRANT_TEAL;
    ILI9341_DrawString(172, 75, val_str, t2_color, TFT_BLACK, 2);
    
    // Vibration
    ILI9341_DrawString(172, 102, "VIB SHOCK:", TFT_LIGHT_GREY, TFT_BLACK, 1);
    int vib_i = (int)vib; int vib_f = (int)((vib - vib_i) * 100); if (vib_f < 0) vib_f = -vib_f;
    sprintf(val_str, "%d.%02d G", vib_i, vib_f);
    uint16_t vib_color = (vib > 1.20f) ? TFT_NEON_RED : ((vib > 0.60f) ? TFT_AMBER : TFT_WHITE);
    ILI9341_DrawString(240, 102, val_str, vib_color, TFT_BLACK, 1);
    
    // --- BOTTOM RIGHT: CO Level ---
    ILI9341_DrawString(172, 143, "CARBON MONOXIDE (CO)", TFT_LIGHT_GREY, TFT_BLACK, 1);
    
    int ppm_i = (int)ppm;
    int ppm_f = (int)((ppm - ppm_i) * 100);
    if (ppm_f < 0) ppm_f = -ppm_f;
    sprintf(val_str, "%3d.%02d PPM", ppm_i, ppm_f);
    
    uint16_t ppm_color = TFT_NEON_GREEN;
    const char* status_str = "SAFE ";
    if (ppm > 50.0f) {
        ppm_color = TFT_NEON_RED;
        status_str = "ALERT";
    } else if (ppm > 15.0f) {
        ppm_color = TFT_AMBER;
        status_str = "WARN ";
    }
    
    ILI9341_DrawString(172, 161, val_str, ppm_color, TFT_BLACK, 2);
    
    // CO Alarm Status
    ILI9341_DrawString(172, 185, "ALARM STATUS:", TFT_LIGHT_GREY, TFT_BLACK, 1);
    ILI9341_DrawString(255, 185, status_str, ppm_color, TFT_BLACK, 1);
}

// --- Page 1: Individual Cell Voltages with progress bars ---
void ILI9341_DrawPage1(float c1, float c2, float c3, float c4) {
    float cells[4] = {c1, c2, c3, c4};
    char label[32];
    
    for (int i = 0; i < 4; i++) {
        uint16_t y = 40 + i * 48;
        
        // Print Cell Label
        sprintf(label, "CELL %d:", i + 1);
        ILI9341_DrawString(15, y, label, TFT_LIGHT_GREY, TFT_BLACK, 1);
        
        // Format Cell Value
        int c_i = (int)cells[i];
        int c_f = (int)((cells[i] - c_i) * 100);
        if (c_f < 0) c_f = -c_f;
        sprintf(label, "%d.%02dV      ", c_i, c_f);
        
        uint16_t color = TFT_NEON_GREEN;
        if (cells[i] < 1.0f) {
            color = TFT_NEON_RED;
            sprintf(label, "0.00V (DISC)");
        } else if (cells[i] < 3.70f) {
            color = TFT_NEON_RED;
        } else if (cells[i] < 3.85f) {
            color = TFT_AMBER;
        } else if (cells[i] > 4.22f) {
            color = TFT_NEON_RED;
        }
        
        ILI9341_DrawString(15, y + 12, label, color, TFT_BLACK, 2);
        
        // Progress Bar (Range 3.7V = 0% to 4.2V = 100%)
        float percentage_f = 0.0f;
        if (cells[i] > 3.70f) {
            percentage_f = (cells[i] - 3.70f) / (4.20f - 3.70f) * 100.0f;
        }
        if (percentage_f > 100.0f) percentage_f = 100.0f;
        if (percentage_f < 0.0f) percentage_f = 0.0f;
        
        uint8_t percentage = (uint8_t)percentage_f;
        
        // Draw the progress bar
        ILI9341_DrawProgressBar(130, y + 6, 170, 18, percentage, color, TFT_DARK_GREY);
    }
}

// --- Page 2: System Diagnostics & Raw ADC pins ---
void ILI9341_DrawPage2(float v_pin1, float v_pin2, float v_pin3, float v_pin4, float curr_pin, float mq7_pin, float vib_val, float ppm) {
    char msg[32];
    
    // Draw dividing line
    ILI9341_FillRect(160, 30, 2, 210, TFT_GRID_GREY);
    
    // --- LEFT COLUMN: ADC Pins ---
    ILI9341_DrawString(10, 36, "RAW ADC TELEMETRY", TFT_VIBRANT_BLUE, TFT_BLACK, 1);
    
    float pins[4] = {v_pin1, v_pin2, v_pin3, v_pin4};
    for(int i = 0; i < 4; i++) {
        int p_i = (int)pins[i];
        int p_f = (int)((pins[i] - p_i) * 1000);
        if (p_f < 0) p_f = -p_f;
        sprintf(msg, "PIN C%d: %d.%03dV", i + 1, p_i, p_f);
        ILI9341_DrawString(10, 52 + i * 22, msg, TFT_WHITE, TFT_BLACK, 1);
    }
    
    // Current PIN
    int cp_i = (int)curr_pin;
    int cp_f = (int)((curr_pin - cp_i) * 1000);
    if (cp_f < 0) cp_f = -cp_f;
    sprintf(msg, "PIN CUR: %d.%03dV", cp_i, cp_f);
    ILI9341_DrawString(10, 148, msg, TFT_VIBRANT_TEAL, TFT_BLACK, 1);
    
    // MQ7 PIN
    int mq_i = (int)mq7_pin;
    int mq_f = (int)((mq7_pin - mq_i) * 1000);
    if (mq_f < 0) mq_f = -mq_f;
    sprintf(msg, "PIN MQ7: %d.%03dV", mq_i, mq_f);
    ILI9341_DrawString(10, 170, msg, TFT_VIBRANT_TEAL, TFT_BLACK, 1);
    
    // --- RIGHT COLUMN: Vibration Gauge & Gas status ---
    ILI9341_DrawString(172, 36, "SHOCK ACCELEROMETER", TFT_VIBRANT_BLUE, TFT_BLACK, 1);
    
    int vib_i = (int)vib_val;
    int vib_f = (int)((vib_val - vib_i) * 100);
    if (vib_f < 0) vib_f = -vib_f;
    sprintf(msg, "ACCEL: %d.%02dG", vib_i, vib_f);
    ILI9341_DrawString(172, 52, msg, TFT_WHITE, TFT_BLACK, 1);
    
    // Vibration bar (0G = 0%, 2.0G = 100%)
    float vib_perc = vib_val / 2.0f * 100.0f;
    if (vib_perc > 100.0f) vib_perc = 100.0f;
    if (vib_perc < 0.0f) vib_perc = 0.0f;
    uint8_t v_perc = (uint8_t)vib_perc;
    
    uint16_t v_color = TFT_NEON_GREEN;
    if (vib_val > 1.2f) v_color = TFT_NEON_RED;
    else if (vib_val > 0.6f) v_color = TFT_AMBER;
    
    ILI9341_DrawProgressBar(172, 70, 134, 12, v_perc, v_color, TFT_DARK_GREY);
    
    // Gas Sensor Status
    ILI9341_DrawString(172, 102, "CO GAS DIAGNOSTIC", TFT_VIBRANT_BLUE, TFT_BLACK, 1);
    
    if (mq7_pin * 2.0f < 0.30f) {
        ILI9341_DrawString(172, 118, "MQ7: DISCONNECTED ", TFT_NEON_RED, TFT_BLACK, 1);
        ILI9341_DrawString(172, 134, "GAS: 0.00 PPM     ", TFT_LIGHT_GREY, TFT_BLACK, 1);
    } else {
        ILI9341_DrawString(172, 118, "MQ7: CONNECTED    ", TFT_NEON_GREEN, TFT_BLACK, 1);
        
        int p_i = (int)ppm;
        int p_f = (int)((ppm - p_i) * 100);
        if (p_f < 0) p_f = -p_f;
        sprintf(msg, "GAS: %d.%02d PPM   ", p_i, p_f);
        ILI9341_DrawString(172, 134, msg, TFT_WHITE, TFT_BLACK, 1);
    }
    
    // System Health Status
    ILI9341_DrawString(172, 166, "SYSTEM HEALTH", TFT_VIBRANT_BLUE, TFT_BLACK, 1);
    
    uint8_t has_fault = (v_pin1 < 1.0f || (mq7_pin * 2.0f < 0.30f) || (vib_val > 1.20f) || (ppm > 50.0f));
    if (has_fault) {
        ILI9341_DrawString(172, 182, "STATUS: ALERT   ", TFT_NEON_RED, TFT_BLACK, 1);
    } else {
        ILI9341_DrawString(172, 182, "STATUS: NOMINAL ", TFT_NEON_GREEN, TFT_BLACK, 1);
    }
}

// Draw the full-page boot logo (320x240 pixels)
void ILI9341_DrawLogo(const uint16_t* logo) {
    ILI9341_SetAddressWindow(0, 0, 319, 239);
    TFT_DC_H();
    TFT_CS_L();
    
    uint32_t total_pixels = 320 * 240;
    for (uint32_t i = 0; i < total_pixels; i++) {
        uint16_t color = logo[i];
        while (!(SPI1->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&SPI1->DR = color >> 8;
        while (!(SPI1->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&SPI1->DR = color & 0xFF;
    }
    SPI1_Wait();
    TFT_CS_H();
}

// Smooth horizontal swipe wipe transition
void ILI9341_WipeTransition(uint16_t accent_color) {
    uint16_t stripe_w = 20;
    for (uint16_t x = 0; x < 320; x += stripe_w) {
        // Draw the swipe bar
        ILI9341_FillRect(x, 0, stripe_w, 240, accent_color);
        // Erase trailing stripe
        if (x >= stripe_w) {
            ILI9341_FillRect(x - stripe_w, 0, stripe_w, 240, TFT_BLACK);
        }
        HAL_Delay(8); // Control sweep speed
    }
    // Erase last stripe
    ILI9341_FillRect(320 - stripe_w, 0, stripe_w, 240, TFT_BLACK);
}

