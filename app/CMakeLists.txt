cmake_minimum_required(VERSION 3.16)

project(App VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
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

# Mark Theme.qml as a QML singleton so the generated qmldir exports it properly
set_source_files_properties(qml/Theme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

# Set singleton properties for QML singletons
set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

# QML Module with refactored backend, models, and UI components
qt_add_qml_module(app
    URI Configurator
    VERSION 1.0
    SOURCES
        include/device_controller.hpp
        src/device_controller.cpp
        include/key_model.hpp
        src/key_model.cpp
        include/hid_transport.hpp
        src/hid_transport.cpp
        include/actions.hpp
        src/actions.cpp
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/components/ConnectionHeader.qml
        qml/components/DeviceMatrixView.qml
        qml/components/KeyConfigPanel.qml
        qml/components/DisplayConfigPanel.qml
    RESOURCES
        assets/icons/play.svg
        assets/icons/volume-1.svg
        assets/icons/volume-2.svg
        assets/icons/volume-x.svg
        assets/icons/fast-forward.svg
        assets/icons/rewind.svg
        assets/icons/square-dashed-plus.svg
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)

target_link_libraries(app
    PRIVATE 
        Qt6::Quick
        Qt6::QuickControls2
        PkgConfig::HIDAPI
)

# ---------------------------------------------------------------------------
# Compiler Warnings & Enforcement
# ---------------------------------------------------------------------------
option(STRICT_WARNINGS "Treat compiler warnings as errors" OFF)
if(STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(app PRIVATE /W4 /WX)
    else()
        target_compile_options(app PRIVATE -Wall -Wextra -Werror)
    endif()
endif()

install(TARGETS app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)