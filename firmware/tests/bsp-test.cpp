#include "bsp/bsp.hpp"
#include <cstdio>

// ---------------------------------------------------------------------------
// 320x240 Full-Screen Layout Configuration
// ---------------------------------------------------------------------------
inline constexpr int16_t BTN_SIZE = 48; // Square tile size for all buttons

// Side buttons (Left column)
inline constexpr int16_t SIDE_BTN_X = 20;
inline constexpr int16_t SIDE1_TEXT_Y = 34;
inline constexpr int16_t SIDE1_BOX_Y = 48;
inline constexpr int16_t SIDE2_TEXT_Y = 138;
inline constexpr int16_t SIDE2_BOX_Y = 152;

// 4x4 Key Matrix (Right area: X = 96..300, Y = 28..232)
inline constexpr int16_t MATRIX_ORIGIN_X = 96;
inline constexpr int16_t MATRIX_ORIGIN_Y = 28;
inline constexpr int16_t MATRIX_SPACING = 52; // 48px box + 4px gap

static void draw_ui(bsp::Display& display) {
    display.clear(bsp::graphics::Color::Black);

    // Title centered-ish at the top with 2x scale (Font: 5x8 -> 10x16 + spacing)
    display.draw_text(26, 6, "MACROPAD TEST", bsp::graphics::Color::White, bsp::graphics::Color::Black, 2);

    // Side Button 1 (Left)
    display.draw_text(SIDE_BTN_X + 7, SIDE1_TEXT_Y, "SIDE 1", bsp::graphics::Color::White, bsp::graphics::Color::Black,
                      1);
    display.fill_rect(SIDE_BTN_X, SIDE1_BOX_Y, BTN_SIZE, BTN_SIZE, bsp::graphics::Color::Gray);

    // Side Button 2 (Left)
    display.draw_text(SIDE_BTN_X + 7, SIDE2_TEXT_Y, "SIDE 2", bsp::graphics::Color::White, bsp::graphics::Color::Black,
                      1);
    display.fill_rect(SIDE_BTN_X, SIDE2_BOX_Y, BTN_SIZE, BTN_SIZE, bsp::graphics::Color::Gray);

    // 4x4 matrix keys
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            int16_t x = MATRIX_ORIGIN_X + (c * MATRIX_SPACING);
            int16_t y = MATRIX_ORIGIN_Y + (r * MATRIX_SPACING);
            display.fill_rect(x, y, BTN_SIZE, BTN_SIZE, bsp::graphics::Color::Gray);
        }
    }
}

int main() {
    auto& board = bsp::Board::instance();
    board.init();

    printf("BSP init complete. Starting full-screen test loop...\n");
    draw_ui(board.display);

    while (true) {
        board.update();

        // 1. Check all 16 Matrix Keys
        for (size_t i = 0; i < 16; ++i) {
            auto key = static_cast<bsp::Key>(i);
            int r = i / 4;
            int c = i % 4;
            int16_t x = MATRIX_ORIGIN_X + (c * MATRIX_SPACING);
            int16_t y = MATRIX_ORIGIN_Y + (r * MATRIX_SPACING);

            if (board.keys.just_pressed(key)) {
                printf("Key [%d, %d] PRESSED\n", r, c);
                board.display.fill_rect(x, y, BTN_SIZE, BTN_SIZE, bsp::graphics::Color::Green);
            } else if (board.keys.just_released(key)) {
                printf("Key [%d, %d] RELEASED\n", r, c);
                board.display.fill_rect(x, y, BTN_SIZE, BTN_SIZE, bsp::graphics::Color::Gray);
            }
        }

        // 2. Check Side Button 1
        if (board.keys.just_pressed(bsp::Key::SideBtn1)) {
            printf("Side Button 1 PRESSED\n");
            board.display.fill_rect(SIDE_BTN_X, SIDE1_BOX_Y, BTN_SIZE, BTN_SIZE, bsp::graphics::Color::Cyan);
        } else if (board.keys.just_released(bsp::Key::SideBtn1)) {
            printf("Side Button 1 RELEASED\n");
            board.display.fill_rect(SIDE_BTN_X, SIDE1_BOX_Y, BTN_SIZE, BTN_SIZE, bsp::graphics::Color::Gray);
        }

        // 3. Check Side Button 2
        if (board.keys.just_pressed(bsp::Key::SideBtn2)) {
            printf("Side Button 2 PRESSED\n");
            board.display.fill_rect(SIDE_BTN_X, SIDE2_BOX_Y, BTN_SIZE, BTN_SIZE, bsp::graphics::Color::Cyan);
        } else if (board.keys.just_released(bsp::Key::SideBtn2)) {
            printf("Side Button 2 RELEASED\n");
            board.display.fill_rect(SIDE_BTN_X, SIDE2_BOX_Y, BTN_SIZE, BTN_SIZE, bsp::graphics::Color::Gray);
        }

        sleep_ms(10);
    }

    return 0;
}