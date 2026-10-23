#pragma once

#include "bsp/board_config.hpp"
#include "bsp/color.hpp"
#include "bsp/display.hpp"
#include "bsp/keyboard.hpp"
#include "pico/stdlib.h"

namespace bsp {

class Board {
public:
    static Board& instance() {
        static Board inst;
        return inst;
    }

    void init() {
        stdio_init_all();
        display.init();
        keys.init();
    }

    void update() {
        keys.update();
    }

    Display  display;
    Keyboard keys;

private:
    Board() = default;
    Board(const Board&) = delete;
    Board& operator=(const Board&) = delete;
};

} // namespace bsp