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
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico-dow-clock C CXX ASM)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(EXISTS "${CMAKE_SOURCE_DIR}/secrets.cmake")
    include(${CMAKE_SOURCE_DIR}/secrets.cmake)
else()
    message(FATAL_ERROR "Missing secrets.cmake for Wi-Fi configuration! Expected contents:\nset(WIFI_SSID \"yourSSID\" CACHE STRING \"\")\nset(WIFI_PASSWORD \"yourPassword\" CACHE STRING \"\")")
endif()

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(lib/Config)
add_subdirectory(lib/LCD) 
add_subdirectory(lib/Fonts)
add_subdirectory(lib/GUI)
add_subdirectory(fontgen)

include_directories(
    ./lib/Fonts
    ./lib/Config
    ./lib/GUI
    ./lib/LCD
    ${CMAKE_CURRENT_LIST_DIR})

add_executable(pico-dow-clock 
    ntp.c
    pico-dow-clock.c
    )

# Configuration options for the clock. Wi-Fi credentials are required
# and found in secrets.cmake
add_compile_definitions(
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    NTP_SERVER=\"uk.pool.ntp.org\"
    DEBUG=1
)

pico_set_program_name(pico-dow-clock "pico-dow-clock")
pico_set_program_version(pico-dow-clock "0.1")

# Add any user requested libraries
target_link_libraries(pico-dow-clock 
        Config
        Fonts
        GUI
        LCD
        pico_stdlib
        hardware_spi
        pico_cyw43_arch_lwip_threadsafe_background
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(pico-dow-clock 1)
pico_enable_stdio_uart(pico-dow-clock 0)

pico_add_extra_outputs(pico-dow-clock)
