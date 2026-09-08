cmake_minimum_required(VERSION 3.16)

project(App VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)
find_package(PkgConfig REQUIRED)

# Look for hidapi-hidraw (or hidapi-libusb depending on what you use)
pkg_check_modules(HIDAPI REQUIRED IMPORTED_TARGET hidapi-hidraw)

qt_standard_project_setup(REQUIRES 6.10)

include(GNUInstallDirs)

qt_add_executable(appApp
    main.cpp
)

qt_add_qml_module(appApp
    URI Configurator
    VERSION 1.0
    SOURCES
        controller.h
        controller.cpp
    QML_FILES
        Main.qml
)

target_link_libraries(appApp
    PRIVATE 
        Qt6::Quick
        Qt6::QuickControls2
        PkgConfig::HIDAPI   # <--- Link hidapi here
)

install(TARGETS appApp
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)