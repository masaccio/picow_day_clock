# ====================================================================================
# Test build (uses native tools)
# ====================================================================================
# We keep this build separate as it only builds in Unix-like hosts and also
# requires host compilers rather than the Pico SDK

cmake_minimum_required(VERSION 3.13)
project(test_picow_day_clock C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

if(APPLE)
    set(COVERAGE_FLAGS "-fprofile-instr-generate -fcoverage-mapping -O0 -g")
elseif(UNIX)
    set(COVERAGE_FLAGS "--coverage -O0 -g")
else()
    message(FATAL_ERROR "Tests can only be built on macOS or UNIX-like systems.")
endif()

file(GLOB CLOCK_SRC_FILES ${CMAKE_SOURCE_DIR}/../../src/*.c)
file(GLOB GENERATED_FONT_SRC_FILES ${CMAKE_SOURCE_DIR}/../../generated/*.c)
file(GLOB TEST_SRC_FILES ${CMAKE_SOURCE_DIR}/*.c)

set(COVERED_CLOCK_SRC_FILES ${CLOCK_SRC_FILES})
set(NON_COVERED_CLOCK_SRC_FILES
    ${CMAKE_SOURCE_DIR}/../../src/fb.c
    ${CMAKE_SOURCE_DIR}/../../src/lcd.c
)
list(REMOVE_ITEM COVERED_CLOCK_SRC_FILES ${NON_COVERED_CLOCK_SRC_FILES})
set_source_files_properties(${COVERED_CLOCK_SRC_FILES} 
    PROPERTIES COMPILE_FLAGS "${COVERAGE_FLAGS}"
)

include_directories(
    ../../src
    ../../generated
)

add_executable(test_picow_day_clock 
    ${COVERED_CLOCK_SRC_FILES}
    ${NON_COVERED_CLOCK_SRC_FILES}
    ${GENERATED_FONT_SRC_FILES}
    ${TEST_SRC_FILES}
)

# We can be even more verbose than the Pico SDK
target_compile_options(test_picow_day_clock PRIVATE -Wall -Wextra -Wpedantic -Werror)

target_compile_definitions(test_picow_day_clock PRIVATE
    WIFI_SSID="my-test-ssid"
    WIFI_PASSWORD="my-test-password"
    NTP_SERVER="ntp.test.invalid"
    CLOCK_DEBUG_ENABLED=0
    TEST_MODE=1
)

target_link_libraries(test_picow_day_clock PRIVATE ${COVERAGE_FLAGS})
