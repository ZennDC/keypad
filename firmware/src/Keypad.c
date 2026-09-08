#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"

#include "config_manager.h"
#include "matrix.h"
#include "backlight.h"

int main(void) {
    board_init();
    config_manager_init();
    matrix_init();
    backlight_init(config_manager_get_display()->brightness);
    tusb_init();

    while (1) {
        tud_task();
        matrix_task();
    }

    return 0;
}