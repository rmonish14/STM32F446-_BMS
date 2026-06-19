#ifndef __ILI9341_H
#define __ILI9341_H

#include "main.h"

// Screen dimensions in landscape mode
#define ILI9341_WIDTH  320
#define ILI9341_HEIGHT 240

// Orientation configuration (MADCTL register - Command 0x36)
// Common values:
// - 0xE8: Standard Landscape Inverted (Recommended for standard ILI9341 red modules)
// - 0x28: Standard Landscape (Alternative ILI9341 landscape)
// - 0x48 / 0x88: Native Landscape panel settings (Set this if 0x28 or 0xE8 displays vertically!)
// - 0x70 / 0xA0 / 0x60: Landscape settings for ST7789 display controller clones
#define TFT_MADCTL_VAL  0x48


// --- Pin configuration (GPIO register helpers) ---
// PA4: CS (Chip Select)
#define TFT_CS_L()   (GPIOA->BSRR = (1U << 20)) // Clear pin PA4 (4 + 16)
#define TFT_CS_H()   (GPIOA->BSRR = (1U << 4))  // Set pin PA4
// PA6: D/C (Data/Command Select)
#define TFT_DC_L()   (GPIOA->BSRR = (1U << 22)) // Clear pin PA6 (6 + 16)
#define TFT_DC_H()   (GPIOA->BSRR = (1U << 6))  // Set pin PA6
// PA1: RST (Reset)
#define TFT_RST_L()  (GPIOA->BSRR = (1U << 17)) // Clear pin PA1 (1 + 16)
#define TFT_RST_H()  (GPIOA->BSRR = (1U << 1))  // Set pin PA1

// RGB565 sleek color palette for dark theme
#define TFT_BLACK       0x0000 // Deep black
#define TFT_DARK_GREY   0x18E3 // Sleek dashboard background
#define TFT_GRID_GREY   0x3186 // Grid borders
#define TFT_WHITE       0xFFFF // Text
#define TFT_LIGHT_GREY  0xC618 // Dimmer text / subtitles
#define TFT_VIBRANT_BLUE 0x03DF // Cool theme blue
#define TFT_VIBRANT_TEAL 0x07FF // Cyan/teal for secondary metrics
#define TFT_NEON_GREEN  0x07E0 // Normal charge / safe zones
#define TFT_AMBER       0xFD20 // Warning levels
#define TFT_NEON_RED    0xF800 // Alarm/fault levels

// Function Prototypes
void ILI9341_SPI_Init(void);
void ILI9341_Init(void);
void ILI9341_WriteCommand(uint8_t cmd);
void ILI9341_WriteData(uint8_t data);
void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ILI9341_FillScreen(uint16_t color);
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);
void ILI9341_DrawString(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size);
void ILI9341_DrawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t percentage, uint16_t bar_color, uint16_t bg_color);

extern const uint16_t boot_logo[320 * 240];

// High-level dashboard layouts & transitions
void ILI9341_WipeTransition(uint16_t accent_color);
void ILI9341_DrawLogo(const uint16_t* logo);
void ILI9341_DrawHeader(const char* title, uint8_t page_num);
void ILI9341_DrawPage0(float pack_v, float current, float t1, float t2, float vib, float ppm, float soc, uint8_t charge_status, uint8_t wifi_status);
void ILI9341_DrawPage1(float c1, float c2, float c3, float c4);
void ILI9341_DrawPage2(float v_pin1, float v_pin2, float v_pin3, float v_pin4, float curr_pin, float mq7_pin, float vib_val, float ppm);

#endif // __ILI9341_H
