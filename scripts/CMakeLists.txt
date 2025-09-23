set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/scripts)
set(FONT_SCRIPT ${SCRIPTS_DIR}/create-font-bitmaps.py)
set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/generated)
set(VENV_DIR ${CMAKE_BINARY_DIR}/scripts_venv)

set(TEXT_TTF ${CMAKE_SOURCE_DIR}/fonts/Roboto-Medium.ttf)
set(TEXT_HEIGHT 20)
set(TEXT_C_NAME text_font)
set(TEXT_C_SRC ${GENERATED_DIR}/${TEXT_C_NAME}.c)

set(LCD_HEIGHT 320)
set(LCD_WIDTH 172)
set(CLOCK_C_NAME clock_digit_font)
set(CLOCK_TTF ${CMAKE_SOURCE_DIR}/fonts/BebasNeue-Regular.ttf)
set(CLOCK_C_SRC ${GENERATED_DIR}/${CLOCK_C_NAME}.c)

set(ICONS_LIST watchdog memory
               wifi wifi_init wifi_timeout wifi_password wifi_connect wifi_error 
               ntp ntp_init ntp_dns ntp_timeout ntp_memory ntp_error)
set(ICON_SIZE 36)
set(ICON_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/create-icon-bitmaps.py)

# GitHub runner's CMake doesn't set WIN32
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(PYTHON_EXECUTABLE ${VENV_DIR}/Scripts/python.exe)
    set(PIL_PACKAGE_DIR ${VENV_DIR}/Lib/site-packages/PIL)
else()
    set(PYTHON_EXECUTABLE ${VENV_DIR}/bin/python3)
    execute_process(
        COMMAND python3 -c "import sys;v=sys.version_info;sys.stdout.write(f'{v.major}.{v.minor}')"
        OUTPUT_VARIABLE PYTHON_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(PIL_PACKAGE_DIR ${VENV_DIR}/lib/python${PYTHON_VERSION}/site-packages/PIL)
endif()

add_custom_command(
    OUTPUT ${PYTHON_EXECUTABLE}
    COMMAND python3 -m venv ${VENV_DIR}
    COMMENT "Creating Python virtual environment for font generation"
)

add_custom_command(
    OUTPUT ${PIL_PACKAGE_DIR}
    DEPENDS ${PYTHON_EXECUTABLE}
    COMMAND ${PYTHON_EXECUTABLE} -m pip install --upgrade pip
    COMMAND ${PYTHON_EXECUTABLE} -m pip install --force Pillow
    COMMENT "Installing Pillow in virtual environment"
)

set(GENERATED_FONT_SRC_FILES "")
set(GENERATED_ICON_SRC_FILES "")

list(APPEND GENERATED_FONT_SRC_FILES ${TEXT_C_SRC})
add_custom_command(
    OUTPUT ${TEXT_C_SRC}
    DEPENDS ${FONT_SCRIPT} ${TEXT_TTF} ${PIL_PACKAGE_DIR}
    COMMAND ${PYTHON_EXECUTABLE} ${FONT_SCRIPT}
            --c-name=${TEXT_C_NAME}
            --height=${TEXT_HEIGHT}
            ${TEXT_TTF} ${TEXT_C_SRC}
    COMMENT "Generating font file ${TEXT_C_SRC}"
)

list(APPEND GENERATED_FONT_SRC_FILES ${CLOCK_C_SRC})
add_custom_command(
    OUTPUT ${CLOCK_C_SRC}
    DEPENDS ${FONT_SCRIPT} ${CLOCK_TTF} ${PIL_PACKAGE_DIR}
    COMMAND ${PYTHON_EXECUTABLE} ${FONT_SCRIPT} 
            --clock
            --c-name=${CLOCK_C_NAME}
            --height=${LCD_HEIGHT}
            --width=${LCD_WIDTH}
            ${CLOCK_TTF} ${CLOCK_C_SRC}
    COMMENT "Generating font file ${CLOCK_C_SRC}"
)

foreach(ICON_NAME IN ITEMS ${ICONS_LIST})
    set(ICON_C_FILE ${GENERATED_DIR}/${ICON_NAME}_icon.c)
    list(APPEND GENERATED_ICON_SRC_FILES ${ICON_C_FILE})

    add_custom_command(
        OUTPUT ${ICON_C_FILE}
        DEPENDS ${ICON_SCRIPT} ${PIL_PACKAGE_DIR}
        COMMAND ${PYTHON_EXECUTABLE} ${ICON_SCRIPT}
            --size=${ICON_SIZE}
            --c-name=${ICON_NAME}_icon
            --output=${ICON_C_FILE}
            ${CMAKE_SOURCE_DIR}/images/${ICON_NAME}-icon.jpeg
        COMMENT "Generating icon file ${ICON_C_FILE}"
    )
endforeach()

add_custom_target(generate_fonts
    DEPENDS ${GENERATED_FONT_SRC_FILES}
    COMMENT "Generating all fonts"
)

add_custom_target(generate_icons
    DEPENDS ${GENERATED_ICON_SRC_FILES}
    COMMENT "Generating all icons"
)

add_custom_target(clean_generated
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${PIL_PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E rm -f
        ${GENERATED_FONT_SRC_FILES}
        ${GENERATED_ICON_SRC_FILES}
    COMMENT "Cleaning generated font and icon files"
)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
    ${GENERATED_FONT_SRC_FILES}
    ${GENERATED_ICON_SRC_FILES}
)

set(GENERATED_FONT_SRC_FILES ${GENERATED_FONT_SRC_FILES} PARENT_SCOPE)
set(GENERATED_ICON_SRC_FILES ${GENERATED_ICON_SRC_FILES} PARENT_SCOPE)
