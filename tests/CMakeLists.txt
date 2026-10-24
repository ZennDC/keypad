cmake_minimum_required(VERSION 3.16)
project(KeypadProtocolTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(protocol_tests
    test_protocol.cpp
)

target_include_directories(protocol_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)

target_link_libraries(protocol_tests PRIVATE
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(protocol_tests)
