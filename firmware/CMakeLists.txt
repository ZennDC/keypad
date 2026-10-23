# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

# ---------------------------------------------------------------------------
# C/C++ Standards & Tooling Exports
# ---------------------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in SDK import script (located in firmware root)
include(cmake/pico_sdk_import.cmake)

project(Keypad C CXX ASM)

pico_sdk_init()

# ---------------------------------------------------------------------------
# Executable & Sources
# ---------------------------------------------------------------------------
add_executable(Keypad
    src/Keypad.c
    src/usb_descriptors.c
    src/flash_storage.c
    src/backlight.c
    src/config_manager.c
    src/matrix.c
)

pico_set_program_name(Keypad "Keypad")
pico_set_program_version(Keypad "0.1")

# Standard I/O routing (USB CDC / UART)
pico_enable_stdio_uart(Keypad 0)
pico_enable_stdio_usb(Keypad 0)

# ---------------------------------------------------------------------------
# Include Directories & Dependencies
# ---------------------------------------------------------------------------
target_include_directories(Keypad PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/usb
    ${CMAKE_CURRENT_LIST_DIR}/../shared/include
)

target_link_libraries(Keypad
    pico_stdlib
    hardware_pwm
    hardware_flash
    tinyusb_device
    tinyusb_board
)

# Generate UF2, HEX, and map files
pico_add_extra_outputs(Keypad)

# ---------------------------------------------------------------------------
# Bsp Test Executable
# ---------------------------------------------------------------------------

add_executable(bsp 
    src/bsp-test.cpp
    src/bsp/display.cpp
    src/bsp/keyboard.cpp
)

pico_set_program_name(bsp "bsp")
pico_set_program_version(bsp "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(bsp 0)
pico_enable_stdio_usb(bsp 1)

# Add the standard library to the build
target_link_libraries(bsp
        pico_stdlib
        hardware_spi
        hardware_gpio
        hardware_dma
)

target_include_directories(bsp PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Add the standard include files to the build
target_include_directories(bsp PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(bsp)

