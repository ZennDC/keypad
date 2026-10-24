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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico CACHE STRING "Board type")

include(cmake/pico_sdk_import.cmake)

project(Keypad C CXX ASM)

pico_sdk_init()

option(BUILD_TESTS "Build hardware test targets" ON)

# ---------------------------------------------------------------------------
# BSP Library
# ---------------------------------------------------------------------------
add_library(bsp STATIC
    src/bsp/display.cpp
    src/bsp/keyboard.cpp
)

target_include_directories(bsp PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(bsp PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_dma
    hardware_pwm
)

# ---------------------------------------------------------------------------
# Main Firmware Executable
# ---------------------------------------------------------------------------
add_executable(Keypad
    src/Keypad.cpp
    src/menu_controller.cpp
    src/usb/usb_descriptors.cpp
    src/flash_storage.cpp
    src/config_manager.cpp
)

pico_set_program_name(Keypad "Keypad")
pico_set_program_version(Keypad "0.1")

pico_enable_stdio_uart(Keypad 0)
pico_enable_stdio_usb(Keypad 0)

target_include_directories(Keypad PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/usb
    ${CMAKE_CURRENT_LIST_DIR}/../shared/include
)

target_link_libraries(Keypad PRIVATE
    bsp
    hardware_pwm
    hardware_flash
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(Keypad)

# ---------------------------------------------------------------------------
# Hardware Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    add_executable(test_bsp
        tests/bsp-test.cpp
    )

    pico_set_program_name(test_bsp "test_bsp")
    pico_set_program_version(test_bsp "0.1")

    # Enable USB serial output for test logs
    pico_enable_stdio_uart(test_bsp 0)
    pico_enable_stdio_usb(test_bsp 1)

    target_link_libraries(test_bsp PRIVATE
        bsp
    )

    pico_add_extra_outputs(test_bsp)
endif()

# ---------------------------------------------------------------------------
# Compiler Warnings & Enforcement
# ---------------------------------------------------------------------------
option(STRICT_WARNINGS "Treat compiler warnings as errors" OFF)
if(STRICT_WARNINGS)
    if(MSVC)
        set(KEYPAD_WARNING_FLAGS /W4 /WX)
    else()
        set(KEYPAD_WARNING_FLAGS -Wall -Wextra -Werror)
    endif()
    target_compile_options(bsp PRIVATE ${KEYPAD_WARNING_FLAGS})
    target_compile_options(Keypad PRIVATE ${KEYPAD_WARNING_FLAGS})
    if(BUILD_TESTS)
        target_compile_options(test_bsp PRIVATE ${KEYPAD_WARNING_FLAGS})
    endif()
endif()