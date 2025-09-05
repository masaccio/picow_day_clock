# Native test CMake project for picow_day_clock

cmake_minimum_required(VERSION 3.13)
project(test_picow_day_clock C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

# Use native compilers explicitly

if(APPLE OR UNIX)
    set(CMAKE_C_COMPILER "/usr/bin/cc" CACHE STRING "" FORCE)
else()
    message(FATAL_ERROR "Tests can only be built on macOS or UNIX-like systems.")
endif()

# Include main project source and generated fonts
file(GLOB CLOCK_SRC_FILES ../src/*.c)
file(GLOB TEST_SRC_FILES ../src/tests/*.c)
file(GLOB GENERATED_FONTS ../generated/*.c)

include_directories(
    ../src
    ../generated
)

add_executable(test_picow_day_clock 
    ${CLOCK_SRC_FILES}
    ${GENERATED_FONTS}
    ${TEST_SRC_FILES}
)

target_compile_options(test_picow_day_clock PRIVATE -Wall -Wextra -Wpedantic -Werror)

target_compile_definitions(test_picow_day_clock PRIVATE
    WIFI_SSID="my-test-ssid"
    WIFI_PASSWORD="my-test-password"
    NTP_SERVER="ntp.test.invalid"
    CLOCK_DEBUG_ENABLED=1
    TEST_MODE=1
)
