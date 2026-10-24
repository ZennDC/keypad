#include "bsp/display.hpp"
#include "bsp/board_config.hpp"
#include "bsp/font5x8.hpp"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

namespace bsp {

using namespace bsp::config::display;

void Display::cs_select() {
    gpio_put(cs_pin, 0);
}

void Display::cs_deselect() {
    gpio_put(cs_pin, 1);
}

void Display::write_cmd(uint8_t cmd) {
    gpio_put(dc_pin, 0);
    cs_select();
    spi_write_blocking(spi_inst, &cmd, 1);
    cs_deselect();
}

void Display::write_data(uint8_t data) {
    gpio_put(dc_pin, 1);
    cs_select();
    spi_write_blocking(spi_inst, &data, 1);
    cs_deselect();
}

void Display::write_data_buffer(const uint8_t* buffer, size_t length) {
    gpio_put(dc_pin, 1);
    cs_select();
    spi_write_blocking(spi_inst, buffer, length);
    cs_deselect();
}

void Display::set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    write_cmd(0x2A); // CASET
    write_data(static_cast<uint8_t>(x0 >> 8));
    write_data(static_cast<uint8_t>(x0 & 0xFF));
    write_data(static_cast<uint8_t>(x1 >> 8));
    write_data(static_cast<uint8_t>(x1 & 0xFF));

    write_cmd(0x2B); // RASET
    write_data(static_cast<uint8_t>(y0 >> 8));
    write_data(static_cast<uint8_t>(y0 & 0xFF));
    write_data(static_cast<uint8_t>(y1 >> 8));
    write_data(static_cast<uint8_t>(y1 & 0xFF));

    write_cmd(0x2C); // RAMWR
}

void Display::init() {
    // 1. SPI initialisatie
    spi_init(spi_inst, spi_baudrate);
    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);

    // 2. Control pins configureren
    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);
    gpio_put(cs_pin, 1);

    gpio_init(dc_pin);
    gpio_set_dir(dc_pin, GPIO_OUT);
    gpio_put(dc_pin, 0);

    gpio_init(rst_pin);
    gpio_set_dir(rst_pin, GPIO_OUT);
    gpio_put(rst_pin, 1);

    gpio_set_function(bl_pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(bl_pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_config_set_wrap(&config, 255);
    pwm_init(slice_num, &config, true);

    // 3. Hardware reset cyclus
    gpio_put(rst_pin, 0);
    sleep_ms(100);
    gpio_put(rst_pin, 1);
    sleep_ms(120);

    // 4. ST7789 commando sequence
    write_cmd(0x11); // Sleep Out
    sleep_ms(120);

    write_cmd(0x3A); // 16-bit RGB565 formaat
    write_data(0x55);

    write_cmd(0x36); // MADCTL orientatie
    write_data(madctl);

    write_cmd(0x21); // Display Inversion ON (standaard voor IPS panelen)
    write_cmd(0x29); // Display ON
    sleep_ms(50);
}

void Display::set_backlight(uint8_t percent) {
    if (percent > 100)
        percent = 100;
    pwm_set_gpio_level(bl_pin, (percent * 255) / 100);
}

void Display::set_sleep(bool sleep) {
    if (sleep) {
        set_backlight(0);
        write_cmd(0x10); // ST7789 Sleep In
    } else {
        write_cmd(0x11); // ST7789 Sleep Out
        sleep_ms(120);
        write_cmd(0x29); // Display ON
    }
}

void Display::fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, graphics::Color color) {
    if (x >= width || y >= height || x + w <= 0 || y + h <= 0)
        return;

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > width)
        w = width - x;
    if (y + h > height)
        h = height - y;

    set_window(x, y, x + w - 1, y + h - 1);

    // Vul een kleine buffer met Big-Endian pixeldata
    uint8_t hi = static_cast<uint8_t>(color.value >> 8);
    uint8_t lo = static_cast<uint8_t>(color.value & 0xFF);

    constexpr size_t chunk_pixels = 32;
    uint8_t buffer[chunk_pixels * 2];
    for (size_t i = 0; i < chunk_pixels; ++i) {
        buffer[i * 2] = hi;
        buffer[i * 2 + 1] = lo;
    }

    gpio_put(dc_pin, 1);
    cs_select();

    uint32_t total_pixels = static_cast<uint32_t>(w) * h;
    while (total_pixels > 0) {
        size_t current_chunk = (total_pixels > chunk_pixels) ? chunk_pixels : total_pixels;
        spi_write_blocking(spi_inst, buffer, current_chunk * 2);
        total_pixels -= current_chunk;
    }

    cs_deselect();
}

void Display::clear(graphics::Color color) {
    fill_rect(0, 0, width, height, color);
}

void Display::draw_bitmap(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint16_t* pixels) {
    if (x >= width || y >= height || pixels == nullptr)
        return;

    set_window(x, y, x + w - 1, y + h - 1);

    // Stream de pixelbuffer direct naar het scherm
    gpio_put(dc_pin, 1);
    cs_select();

    // Pixeldata moet in Big-Endian gestuurd worden
    // Als de bron al BE is, kan het in één write; anders per chunk swappen:
    constexpr size_t chunk_size = 64;
    uint8_t buffer[chunk_size * 2];
    size_t total = static_cast<size_t>(w) * h;
    size_t offset = 0;

    while (offset < total) {
        size_t count = (total - offset > chunk_size) ? chunk_size : (total - offset);
        for (size_t i = 0; i < count; ++i) {
            uint16_t p = pixels[offset + i];
            buffer[i * 2] = static_cast<uint8_t>(p >> 8);
            buffer[i * 2 + 1] = static_cast<uint8_t>(p & 0xFF);
        }
        spi_write_blocking(spi_inst, buffer, count * 2);
        offset += count;
    }

    cs_deselect();
}

void Display::draw_char(int16_t x, int16_t y, char c, graphics::Color fg, graphics::Color bg, uint8_t scale) {
    const auto* glyph = graphics::Font5x8::get_glyph(c);
    if (!glyph)
        return;

    uint16_t total_w = 6 * scale; // 5 kolommen + 1 spatiekolom
    uint16_t total_h = 8 * scale;

    if (x + total_w <= 0 || x >= width || y + total_h <= 0 || y >= height)
        return;

    set_window(x, y, x + total_w - 1, y + total_h - 1);

    uint8_t fg_hi = static_cast<uint8_t>(fg.value >> 8);
    uint8_t fg_lo = static_cast<uint8_t>(fg.value & 0xFF);
    uint8_t bg_hi = static_cast<uint8_t>(bg.value >> 8);
    uint8_t bg_lo = static_cast<uint8_t>(bg.value & 0xFF);

    gpio_put(dc_pin, 1);
    cs_select();

    // Stream alle rijen en kolommen in één window door
    for (int row = 0; row < 8; ++row) {
        for (uint8_t sy = 0; sy < scale; ++sy) {
            for (int col = 0; col < 5; ++col) {
                bool is_fg = ((*glyph)[col] & (1 << row)) != 0;
                uint8_t hi = is_fg ? fg_hi : bg_hi;
                uint8_t lo = is_fg ? fg_lo : bg_lo;

                for (uint8_t sx = 0; sx < scale; ++sx) {
                    uint8_t px[2] = {hi, lo};
                    spi_write_blocking(spi_inst, px, 2);
                }
            }
            // Spatiekolom aan de rechterkant
            for (uint8_t sx = 0; sx < scale; ++sx) {
                uint8_t px[2] = {bg_hi, bg_lo};
                spi_write_blocking(spi_inst, px, 2);
            }
        }
    }

    cs_deselect();
}

void Display::draw_text(int16_t x, int16_t y, std::string_view text, graphics::Color fg, graphics::Color bg,
                        uint8_t scale) {
    int16_t cursor_x = x;
    for (char c : text) {
        draw_char(cursor_x, y, c, fg, bg, scale);
        cursor_x += static_cast<int16_t>(6 * scale);
    }
}

} // namespace bsp