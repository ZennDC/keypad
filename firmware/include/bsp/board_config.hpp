#pragma once

#include <cstdint>
#include <array>
#include "hardware/spi.h"

namespace bsp::config {

// --- Display Configuration ---
namespace display {
    inline constexpr uint cs_pin    = 11;
    inline constexpr uint dc_pin    = 12;
    inline constexpr uint rst_pin   = 13;
    inline constexpr uint sck_pin   = 14; // SCL
    inline constexpr uint mosi_pin  = 15; // SDA
    inline constexpr uint bl_pin    = 10;

    inline spi_inst_t *const spi_inst = spi1;
    inline constexpr uint32_t spi_baudrate = 40 * 1000 * 1000; // 40 MHz

    inline constexpr uint16_t width  = 320;
    inline constexpr uint16_t height = 240;

    // ST7789 MADCTL register value for 90-degree landscape mode
    inline constexpr uint8_t madctl = 0x70;
}

// --- Key Matrix Configuration ---
namespace matrix {
    inline constexpr size_t num_cols = 4;
    inline constexpr size_t num_rows = 4;

    // Col pins driven as OUTPUT, Row pins scanned as INPUT with pull-ups
    inline constexpr std::array<uint, num_cols> col_pins = {2, 3, 4, 5};
    inline constexpr std::array<uint, num_rows> row_pins = {6, 7, 8, 9};

    // Settling time in microseconds after driving column before sampling rows
    inline constexpr uint32_t settle_time_us = 5;
}

// --- Dedicated Buttons Configuration ---
namespace buttons {
    inline constexpr uint btn_side_1_pin = 28;
    inline constexpr uint btn_side_2_pin = 29;
}

// --- Compile-Time Hardware Invariants ---
static_assert(display::width > 0 && display::height > 0, "Display dimensions must be greater than 0");
static_assert(matrix::num_cols == matrix::col_pins.size(), "Col pins size mismatch");
static_assert(matrix::num_rows == matrix::row_pins.size(), "Row pins size mismatch");

} // namespace bsp::config