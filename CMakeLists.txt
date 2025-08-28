# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(picow_day_clock C CXX ASM)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(EXISTS "${CMAKE_SOURCE_DIR}/secrets.cmake")
    include(${CMAKE_SOURCE_DIR}/secrets.cmake)
else()
    message(FATAL_ERROR "Missing secrets.cmake for Wi-Fi configuration! Expected contents:\nset(WIFI_SSID \"yourSSID\" CACHE STRING \"\")\nset(WIFI_PASSWORD \"yourPassword\" CACHE STRING \"\")")
endif()

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(fontgen)

include_directories(
    waveshare/Fonts
    waveshare/Config
    waveshare/GUI
    waveshare/LCD
    src
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_CURRENT_LIST_DIR})

file(GLOB SRC_FILES src/*.c)
file(GLOB FONT_FILES waveshare/Fonts/*.c)
add_executable(picow_day_clock 
    waveshare/Config/DEV_Config.c
    waveshare/LCD/LCD_1in47.c
    waveshare/GUI/GUI_Paint.c
    ${SRC_FILES}
    ${FONT_FILES}
    ${GENERATED_FONTS}
    )

add_dependencies(picow_day_clock generate_fonts)

# Configuration options for the clock. Wi-Fi credentials are required
# and found in secrets.cmake
target_compile_definitions(picow_day_clock PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    NTP_SERVER=\"uk.pool.ntp.org\"
    PICO_CYW43_ARCH_DEBUG_ENABLED=0     # CYW43 Wi-Fi debug
    DEBUG=0                             # Waveshare debug
    CLOCK_DEBUG_ENABLED=1               # Clock debug
)

pico_set_program_name(picow_day_clock "picow_day_clock")
pico_set_program_version(picow_day_clock "0.1")

# Add any user requested libraries
target_link_libraries(picow_day_clock 
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        hardware_pwm
        hardware_i2c
        hardware_spi
        )

pico_add_extra_outputs(picow_day_clock)
