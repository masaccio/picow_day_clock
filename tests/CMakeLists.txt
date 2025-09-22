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

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_C_COMPILER "clang")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_C_COMPILER "clang-cl")
else()
    message(FATAL_ERROR "Unsupported test host ${CMAKE_HOST_SYSTEM_NAME}")
endif()
set(COVERAGE_FLAGS "-fprofile-instr-generate -fcoverage-mapping -O0 -g")

file(GLOB CLOCK_SRC_FILES ${CMAKE_SOURCE_DIR}/../src/*.c)
file(GLOB GENERATED_SRC_FILES ${CMAKE_SOURCE_DIR}/../generated/*.c)
file(GLOB TEST_SRC_FILES ${CMAKE_SOURCE_DIR}/*.c)

set(COVERED_CLOCK_SRC_FILES ${CLOCK_SRC_FILES})
set(NON_COVERED_CLOCK_SRC_FILES
    ${CMAKE_SOURCE_DIR}/../src/fb.c
    ${CMAKE_SOURCE_DIR}/../src/lcd.c
)
list(REMOVE_ITEM COVERED_CLOCK_SRC_FILES ${NON_COVERED_CLOCK_SRC_FILES})
set_source_files_properties(${COVERED_CLOCK_SRC_FILES} 
    PROPERTIES COMPILE_FLAGS "${COVERAGE_FLAGS}"
)

include_directories(
    ${CMAKE_SOURCE_DIR}/../tests
    ${CMAKE_SOURCE_DIR}/../src
    ${CMAKE_SOURCE_DIR}/../generated
)

add_executable(test_picow_day_clock 
    ${COVERED_CLOCK_SRC_FILES}
    ${NON_COVERED_CLOCK_SRC_FILES}
    ${GENERATED_SRC_FILES}
    ${TEST_SRC_FILES}
)

target_compile_options(test_picow_day_clock PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-\#pragma-messages)

target_compile_definitions(test_picow_day_clock PRIVATE
    WIFI_SSID="my-test-ssid"
    WIFI_PASSWORD="my-test-password"
    NTP_SERVER="ntp.test.invalid"
    TEST_MODE=1
)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
target_compile_definitions(test_picow_day_clock PRIVATE
    _CRT_SECURE_NO_WARNINGS     # Ignore Windows secure CRT deprecation warnings
)
endif()

target_link_libraries(test_picow_day_clock PRIVATE ${COVERAGE_FLAGS})
