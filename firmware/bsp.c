#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <string.h>
#include <stdio.h>

// --- Screen Pins ---
#define PIN_BL 10
#define PIN_CS 11
#define PIN_DC 12
#define PIN_RST 13
#define PIN_MOSI 15 // SDA
#define PIN_SCK 14  // SCL
#define SPI_PORT spi1

// Swapped for 90-degree landscape mode
#define LCD_WIDTH  320
#define LCD_HEIGHT 240

// --- Matrix Configuration ---
const uint COL_PINS[] = {2, 3, 4, 5};
const uint ROW_PINS[] = {6, 7, 8, 9};
#define NUM_COLS 4
#define NUM_ROWS 4

// --- Dedicated Button Pins ---
#define BTN_1_PIN 28
#define BTN_2_PIN 29

// --- Color Definitions (RGB565) ---
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define GRAY 0x4208
#define CYAN 0x07FF

// --- Minimal 5x8 ASCII Font (Characters 32 to 90: Space, Symbols, Numbers, A-Z) ---
static const uint8_t font5x8[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
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
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
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
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

// --- Low-level Display Functions ---
static inline void lcd_cs_select() { gpio_put(PIN_CS, 0); }
static inline void lcd_cs_deselect() { gpio_put(PIN_CS, 1); }

static void write_cmd(uint8_t cmd)
{
    gpio_put(PIN_DC, 0);
    lcd_cs_select();
    spi_write_blocking(SPI_PORT, &cmd, 1);
    lcd_cs_deselect();
}

static void write_data(uint8_t data)
{
    gpio_put(PIN_DC, 1);
    lcd_cs_select();
    spi_write_blocking(SPI_PORT, &data, 1);
    lcd_cs_deselect();
}

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    write_cmd(0x2A);
    write_data(x0 >> 8);
    write_data(x0 & 0xFF);
    write_data(x1 >> 8);
    write_data(x1 & 0xFF);

    write_cmd(0x2B);
    write_data(y0 >> 8);
    write_data(y0 & 0xFF);
    write_data(y1 >> 8);
    write_data(y1 & 0xFF);

    write_cmd(0x2C);
}

static void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;
    if (x + w > LCD_WIDTH)
        w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT)
        h = LCD_HEIGHT - y;

    set_window(x, y, x + w - 1, y + h - 1);
    gpio_put(PIN_DC, 1);
    lcd_cs_select();

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint8_t buf[64];
    for (int i = 0; i < 32; i++)
    {
        buf[i * 2] = hi;
        buf[i * 2 + 1] = lo;
    }

    uint32_t total = (uint32_t)w * h;
    while (total > 0)
    {
        uint32_t chunk = (total > 32) ? 32 : total;
        spi_write_blocking(SPI_PORT, buf, chunk * 2);
        total -= chunk;
    }
    lcd_cs_deselect();
}

static void fill_screen(uint16_t color)
{
    fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

// Draw a single scaled character (scale = pixel multiplier)
static void draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale)
{
    if (c < 32 || c > 90)
        return;
    const uint8_t *bitmap = font5x8[c - 32];

    for (int col = 0; col < 5; col++)
    {
        uint8_t line = bitmap[col];
        for (int row = 0; row < 8; row++)
        {
            uint16_t color = (line & (1 << row)) ? fg : bg;
            fill_rect(x + (col * scale), y + (row * scale), scale, scale, color);
        }
    }
    fill_rect(x + (5 * scale), y, scale, 8 * scale, bg);
}

// Draw a string
static void draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t scale)
{
    while (*str)
    {
        draw_char(x, y, *str, fg, bg, scale);
        x += (6 * scale);
        str++;
    }
}

static void st7789_init()
{
    gpio_put(PIN_RST, 0);
    sleep_ms(100);
    gpio_put(PIN_RST, 1);
    sleep_ms(120);

    write_cmd(0x11); // Sleep Out
    sleep_ms(120);

    write_cmd(0x3A); // 16-bit color (RGB565)
    write_data(0x55);

    write_cmd(0x36); // MADCTL - 90 degree rotation
    write_data(0x70); // Use 0x70 for 90-deg clockwise (or 0xA0 if flipped upside down)

    write_cmd(0x21); // Inversion ON (Standard for IPS)
    write_cmd(0x29); // Display ON
    sleep_ms(50);
}

// --- Matrix Setup & Scanning ---
static void matrix_init()
{
    for (int c = 0; c < NUM_COLS; c++)
    {
        gpio_init(ROW_PINS[c]);
        gpio_set_dir(ROW_PINS[c], GPIO_OUT);
        gpio_put(ROW_PINS[c], 1);
    }

    for (int r = 0; r < NUM_ROWS; r++)
    {
        gpio_init(COL_PINS[r]);
        gpio_set_dir(COL_PINS[r], GPIO_IN);
        gpio_pull_up(COL_PINS[r]);
    }
}

static bool matrix_scan(int *out_row, int *out_col)
{
    for (int r = 0; r < NUM_ROWS; r++)
    {
        gpio_put(ROW_PINS[r], 0);
        sleep_us(5);

        for (int c = 0; c < NUM_COLS; c++)
        {
            if (gpio_get(COL_PINS[c]) == 0)
            {
                *out_row = r;
                *out_col = c;
                gpio_put(ROW_PINS[r], 1);
                return true;
            }
        }
        gpio_put(ROW_PINS[r], 1);
    }
    return false;
}

int main()
{
    stdio_init_all();

    // SPI Setup (40 MHz)
    spi_init(SPI_PORT, 40 * 1000 * 1000);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_put(PIN_DC, 1);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1);
    gpio_init(PIN_BL);
    gpio_set_dir(PIN_BL, GPIO_OUT);
    gpio_put(PIN_BL, 1);

    st7789_init();
    matrix_init();

    gpio_init(BTN_1_PIN);
    gpio_set_dir(BTN_1_PIN, GPIO_IN);
    gpio_pull_up(BTN_1_PIN);

    gpio_init(BTN_2_PIN);
    gpio_set_dir(BTN_2_PIN, GPIO_IN);
    gpio_pull_up(BTN_2_PIN);

    // Initial screen layout adjusted for 320x240
    fill_screen(BLACK);
    draw_string(60, 30, "KEY MATRIX TEST", WHITE, BLACK, 2);
    draw_string(95, 75, "STATUS:", GRAY, BLACK, 2);
    draw_string(70, 115, "NO KEY PRESSED", GRAY, BLACK, 2);

    int last_r = -1, last_c = -1;
    bool last_b1 = false, last_b2 = false;
    char text_buf[32];

    while (1)
    {
        int pressed_r = -1;
        int pressed_c = -1;
        bool matrix_pressed = matrix_scan(&pressed_r, &pressed_c);

        bool b1_pressed = (gpio_get(BTN_1_PIN) == 0);
        bool b2_pressed = (gpio_get(BTN_2_PIN) == 0);

        bool state_changed = (matrix_pressed && (pressed_r != last_r || pressed_c != last_c)) ||
                             (b1_pressed != last_b1) ||
                             (b2_pressed != last_b2) ||
                             (!matrix_pressed && (last_r != -1));

        if (state_changed)
        {
            last_r = pressed_r;
            last_c = pressed_c;
            last_b1 = b1_pressed;
            last_b2 = b2_pressed;

            // Clear previous reading area across width 320
            fill_rect(0, 110, LCD_WIDTH, 120, BLACK);

            if (b1_pressed || b2_pressed)
            {
                if (b1_pressed && b2_pressed)
                {
                    draw_string(55, 120, "BTN 1 & 2 PRESSED", CYAN, BLACK, 2);
                    draw_string(50, 160, "GP28 & GP29 -> GND", WHITE, BLACK, 2);
                }
                else if (b1_pressed)
                {
                    draw_string(65, 120, "BUTTON 1 PRESSED", CYAN, BLACK, 2);
                    draw_string(85, 160, "GP29 -> GND", WHITE, BLACK, 2);
                }
                else
                {
                    draw_string(65, 120, "BUTTON 2 PRESSED", CYAN, BLACK, 2);
                    draw_string(85, 160, "GP28 -> GND", WHITE, BLACK, 2);
                }
            }
            else if (matrix_pressed)
            {
                snprintf(text_buf, sizeof(text_buf), "ROW: %d  COL: %d", pressed_r, pressed_c);
                draw_string(55, 120, text_buf, GREEN, BLACK, 2);

                snprintf(text_buf, sizeof(text_buf), "GP%d -> GP%d", ROW_PINS[pressed_r], COL_PINS[pressed_c]);
                draw_string(70, 160, text_buf, WHITE, BLACK, 2);
            }
            else
            {
                draw_string(110, 135, "RELEASED", GRAY, BLACK, 2);
            }
        }

        sleep_ms(20);
    }
}