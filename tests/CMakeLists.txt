# ====================================================================================
# Test build (uses native tools)
# ====================================================================================
# We keep this build separate as it only builds in Unix-like hosts and also
# requires host compilers rather than the Pico SDK

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "Host builds for testing are unsupported on Windows")
endif()

cmake_minimum_required(VERSION 3.13)
project(test_picow_day_clock C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

file(GLOB CLOCK_SRC_FILES ${CMAKE_SOURCE_DIR}/../src/*.c)
list(REMOVE_ITEM CLOCK_SRC_FILES ${CMAKE_SOURCE_DIR}/../src/lcd.c)
file(GLOB GENERATED_SRC_FILES ${CMAKE_SOURCE_DIR}/../generated/*.c)
file(GLOB TEST_SRC_FILES ${CMAKE_SOURCE_DIR}/*.c)

set(SANITIZER_FLAGS "-fsanitize=address -g -fno-omit-frame-pointer")
set(COVERAGE_FLAGS "-fprofile-instr-generate -fcoverage-mapping -O0 -g")

set(CLOCK_SRC_FILES ${CLOCK_SRC_FILES})
# LCD code is too hardware-specific to test
list(REMOVE_ITEM CLOCK_SRC_FILES ${CMAKE_SOURCE_DIR}/../src/lcd.c)
list(REMOVE_ITEM CLOCK_SRC_FILES ${CMAKE_SOURCE_DIR}/../src/pico_main.c)
set_source_files_properties(${CLOCK_SRC_FILES} 
    PROPERTIES COMPILE_FLAGS "${COVERAGE_FLAGS} ${SANITIZER_FLAGS}"
)

include_directories(
    ${CMAKE_SOURCE_DIR}/../tests
    ${CMAKE_SOURCE_DIR}/../src
    ${CMAKE_SOURCE_DIR}/../src/dhcpserver
    ${CMAKE_SOURCE_DIR}/../src/dnsserver
    ${CMAKE_SOURCE_DIR}/../generated
)

add_executable(test_picow_day_clock 
    ${CLOCK_SRC_FILES}
    ${GENERATED_SRC_FILES}
    ${TEST_SRC_FILES}
)

target_compile_options(test_picow_day_clock
    PRIVATE
    -Werror
    -Weverything
    # Disable unacceptable warnings for host build
    -Wno-unknown-warning-option         # Handle Clang vs GCC mismatches
    -Wno-switch-default                 # Conflicts with -Wswitch-enum
    -Wno-declaration-after-statement    # Pointless limitation when using modern C
    -Wno-poison-system-directories      # Host build requires system directories
    -Wno-padded                         # Padding is harmless
    -Wno-unsafe-buffer-usage            # No sensible way to manage buffers with this
    -Wno-allocator-wrappers             # Unhelpful for mock calloc()
)

target_compile_definitions(test_picow_day_clock PRIVATE
    WIFI_SSID="my-test-ssid"
    WIFI_PASSWORD="my-test-password"
    NTP_SERVER="ntp.test.invalid"
    TEST_MODE=1
)

target_link_libraries(
    test_picow_day_clock
    PRIVATE
    ${COVERAGE_FLAGS}
    ${SANITIZER_FLAGS}
)
