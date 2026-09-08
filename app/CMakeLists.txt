cmake_minimum_required(VERSION 3.16)

project(App VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)
find_package(PkgConfig REQUIRED)

# Locate HIDAPI (using hidraw backend on Linux)
pkg_check_modules(HIDAPI REQUIRED IMPORTED_TARGET hidapi-hidraw)

qt_standard_project_setup(REQUIRES 6.10)

include(GNUInstallDirs)

# Main executable target
qt_add_executable(app
    src/main.cpp
)

# QML Module with controller backend
qt_add_qml_module(app
    URI Configurator
    VERSION 1.0
    SOURCES
        include/controller.h
        src/controller.cpp
    QML_FILES
        src/Main.qml
    RESOURCES
        assets/icons/play.svg
        assets/icons/volume-1.svg
        assets/icons/volume-2.svg
        assets/icons/volume-x.svg
        assets/icons/fast-forward.svg
        assets/icons/rewind.svg
)

# Header search paths (Local includes + Root shared protocol)
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/include
)

# Link Qt and HIDAPI libraries
target_link_libraries(app
    PRIVATE 
        Qt6::Quick
        Qt6::QuickControls2
        PkgConfig::HIDAPI
)

install(TARGETS app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)