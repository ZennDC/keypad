#pragma once

#include <stdint.h>

namespace bsp::graphics {

struct Color {
    uint16_t value{0x0000};

    constexpr Color() = default;
    constexpr explicit Color(uint16_t raw) : value(raw) {}

    // Compile-time RGB888 to RGB565 conversion
    static constexpr Color from_rgb(uint8_t r, uint8_t g, uint8_t b) {
        uint16_t rgb565 = static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
        return Color{rgb565};
    }

    // Helper for SPI byte streaming if needed (Big-Endian format)
    [[nodiscard]] constexpr uint16_t to_be() const { return static_cast<uint16_t>((value >> 8) | (value << 8)); }

    // Predefined standard colors
    static const Color Black;
    static const Color White;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;
    static const Color Gray;
};

// Inline definitions so they take no RAM and live in Flash
inline constexpr Color Color::Black{0x0000};
inline constexpr Color Color::White{0xFFFF};
inline constexpr Color Color::Red{0xF800};
inline constexpr Color Color::Green{0x07E0};
inline constexpr Color Color::Blue{0x001F};
inline constexpr Color Color::Yellow{0xFFE0};
inline constexpr Color Color::Cyan{0x07FF};
inline constexpr Color Color::Magenta{0xF81F};
inline constexpr Color Color::Gray{0x4208};

} // namespace bsp::graphics