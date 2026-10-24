#pragma once

#include "bsp/color.hpp"
#include <stdint.h>
#include <string_view>

namespace bsp {

class Display {
public:
    Display() = default;

    // Initialiseert SPI, GPIO's en stuurt de ST7789 configuratie-reeks
    void init();

    // Scherm vullen met een egale kleur
    void clear(graphics::Color color = graphics::Color::Black);

    // Tekent een gevulde rechthoek
    void fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, graphics::Color color);

    // Streamt een raw RGB565 pixelbuffer naar een rechthoekig gebied (voor cover art / afbeeldingen)
    void draw_bitmap(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint16_t* pixels);

    // Tekent een enkel karakter geschaald (scale 1 = 5x8, scale 2 = 10x16, etc.)
    void draw_char(int16_t x, int16_t y, char c, graphics::Color fg, graphics::Color bg, uint8_t scale = 1);

    // Tekent een tekststring
    void draw_text(int16_t x, int16_t y, std::string_view text, graphics::Color fg = graphics::Color::White,
                   graphics::Color bg = graphics::Color::Black, uint8_t scale = 1);

    // Scherm aan/uit of helderheid aanpassen via PWM (0-100%)
    void set_backlight(uint8_t percent);

    // Scherm in/uit slaapstand schakelen
    void set_sleep(bool sleep);

private:
    void write_cmd(uint8_t cmd);
    void write_data(uint8_t data);
    void write_data_buffer(const uint8_t* buffer, size_t length);
    void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    void cs_select();
    void cs_deselect();
};

} // namespace bsp