set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/scripts)
set(FONT_SCRIPT ${SCRIPTS_DIR}/create-font-bitmaps.py)
set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/generated)
set(VENV_DIR ${CMAKE_BINARY_DIR}/scripts_venv)

set(TEXT_TTF ${CMAKE_SOURCE_DIR}/fonts/Roboto-Medium.ttf)
set(TEXT_HEIGHT 20)
set(TEXT_C_NAME text_font)
set(TEXT_C_SRC ${GENERATED_DIR}/${TEXT_C_NAME}.c)

set(HTML_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/create-html-blob.py)
set(HTML_SOURCE ${CMAKE_SOURCE_DIR}/src/html/form.html)
set(HTML_C_NAME html_form)
set(HTML_C_SRC ${GENERATED_DIR}/${HTML_C_NAME}.c)

set(LCD_HEIGHT 320)
set(LCD_WIDTH 172)
set(CLOCK_C_NAME clock_digit_font)
set(CLOCK_TTF ${CMAKE_SOURCE_DIR}/fonts/Oswald.ttf)
set(CLOCK_C_SRC ${GENERATED_DIR}/${CLOCK_C_NAME}.c)

set(ICONS_LIST watchdog watchdog_ntp watchdog_wifi memory
               wifi wifi_init wifi_timeout wifi_password wifi_connect wifi_error
               ntp ntp_init ntp_dns ntp_timeout ntp_memory ntp_error)
set(ICON_SIZE 36)
set(ICON_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/create-icon-bitmaps.py)

# GitHub runner's CMake doesn't set WIN32
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(PYTHON_EXECUTABLE ${VENV_DIR}/Scripts/python.exe)
else()
    set(PYTHON_EXECUTABLE ${VENV_DIR}/bin/python3)
    execute_process(
        COMMAND python3 -c "import sys;v=sys.version_info;sys.stdout.write(f'{v.major}.{v.minor}')"
        OUTPUT_VARIABLE PYTHON_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

set(VENV_STAMP ${VENV_DIR}/.venv-ready)

add_custom_command(
    OUTPUT ${VENV_STAMP}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${VENV_DIR}
    COMMAND python3 -m venv ${VENV_DIR}
    COMMAND ${PYTHON_EXECUTABLE} -m pip install --upgrade pip
    COMMAND ${PYTHON_EXECUTABLE} -m pip install --force Pillow
    COMMAND ${CMAKE_COMMAND} -E touch ${VENV_STAMP}
    COMMENT "Creating Python virtual environment"
)

set(GENERATED_FONT_SRC_FILES "")
set(GENERATED_ICON_SRC_FILES "")
set(GENERATED_HTML_SRC_FILES "")

list(APPEND GENERATED_FONT_SRC_FILES ${TEXT_C_SRC})
add_custom_command(
    OUTPUT ${TEXT_C_SRC}
    DEPENDS ${FONT_SCRIPT} ${TEXT_TTF} ${VENV_STAMP}
    COMMAND ${PYTHON_EXECUTABLE} ${FONT_SCRIPT}
            --c-name=${TEXT_C_NAME}
            --height=${TEXT_HEIGHT}
            ${TEXT_TTF} ${TEXT_C_SRC}
    COMMENT "Generating font file ${TEXT_C_SRC}"
)

list(APPEND GENERATED_FONT_SRC_FILES ${CLOCK_C_SRC})
add_custom_command(
    OUTPUT ${CLOCK_C_SRC}
    DEPENDS ${FONT_SCRIPT} ${CLOCK_TTF} ${VENV_STAMP}
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
        DEPENDS ${ICON_SCRIPT} ${VENV_STAMP}
        COMMAND ${PYTHON_EXECUTABLE} ${ICON_SCRIPT}
            --size=${ICON_SIZE}
            --c-name=${ICON_NAME}_icon
            --output=${ICON_C_FILE}
            ${CMAKE_SOURCE_DIR}/images/${ICON_NAME}-icon.jpeg
        COMMENT "Generating icon file ${ICON_C_FILE}"
    )
endforeach()

list(APPEND GENERATED_HTML_SRC_FILES ${HTML_C_SRC})
add_custom_command(
    OUTPUT ${HTML_C_SRC}
    DEPENDS ${HTML_SCRIPT} ${HTML_SOURCE} ${VENV_STAMP}
    COMMAND ${PYTHON_EXECUTABLE} ${HTML_SCRIPT}
            ${HTML_SOURCE} ${HTML_C_SRC}
    COMMENT "Generating HTML blob ${HTML_C_SRC}"
)

add_custom_target(generate_fonts
    DEPENDS ${GENERATED_FONT_SRC_FILES}
    COMMENT "Generating all fonts"
)

add_custom_target(generate_icons
    DEPENDS ${GENERATED_ICON_SRC_FILES}
    COMMENT "Generating all icons"
)

add_custom_target(generate_html
    DEPENDS ${GENERATED_HTML_SRC_FILES}
    COMMENT "Generating all HTML"
)

add_custom_target(clean_generated
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${VENV_DIR}
    COMMAND ${CMAKE_COMMAND} -E rm -f
        ${GENERATED_FONT_SRC_FILES}
        ${GENERATED_ICON_SRC_FILES}
        ${GENERATED_HTML_SRC_FILES}
    COMMENT "Cleaning generated files"
)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
    ${GENERATED_FONT_SRC_FILES}
    ${GENERATED_ICON_SRC_FILES}
    ${GENERATED_HTML_SRC_FILES}
)

set(GENERATED_FONT_SRC_FILES ${GENERATED_FONT_SRC_FILES} PARENT_SCOPE)
set(GENERATED_ICON_SRC_FILES ${GENERATED_ICON_SRC_FILES} PARENT_SCOPE)
set(GENERATED_HTML_SRC_FILES ${GENERATED_HTML_SRC_FILES} PARENT_SCOPE)
