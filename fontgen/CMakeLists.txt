set(FONTGEN_DIR ${CMAKE_SOURCE_DIR}/fontgen)
set(FONT_SCRIPT ${FONTGEN_DIR}/create-font-bitmaps.py)
set(FONT_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/generated)
set(VENV_DIR ${CMAKE_BINARY_DIR}/fontgen_venv)

set(TEXT_TTF ${FONTGEN_DIR}/Roboto-Medium.ttf)
set(TEXT_HEIGHT 20)
set(TEXT_C_NAME text_font)
set(TEXT_C_SRC ${FONT_OUTPUT_DIR}/${TEXT_C_NAME}.c)

set(LCD_HEIGHT 320)
set(LCD_WIDTH 172)
set(CLOCK_C_NAME clock_digit_font)
set(CLOCK_TTF ${FONTGEN_DIR}/BebasNeue-Regular.ttf)
set(CLOCK_C_SRC ${FONT_OUTPUT_DIR}/${CLOCK_C_NAME}.c)

message(STATUS "---- Platform detection ----")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_VERSION = ${CMAKE_SYSTEM_VERSION}")
message(STATUS "CMAKE_HOST_SYSTEM_NAME = ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "CMAKE_HOST_SYSTEM_VERSION = ${CMAKE_HOST_SYSTEM_VERSION}")
message(STATUS "WIN32 = ${WIN32}")
message(STATUS "UNIX = ${UNIX}")
message(STATUS "MSVC = ${MSVC}")
message(STATUS "MINGW = ${MINGW}")
message(STATUS "CYGWIN = ${CYGWIN}")
message(STATUS "CMAKE_C_COMPILER_ID = ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_GENERATOR = ${CMAKE_GENERATOR}")
message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message(STATUS "---- End platform detection ----")
# GitHub runner's CMake doesn't set WIN32
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
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

# Step 1: Create Python virtual environment
add_custom_command(
    OUTPUT ${PYTHON_EXECUTABLE}
    COMMAND python3 -m venv ${VENV_DIR}
    COMMENT "Creating Python virtual environment for font generation"
)

# Step 2: Install Pillow inside the venv
add_custom_command(
    OUTPUT ${PIL_PACKAGE_DIR}
    DEPENDS ${PYTHON_EXECUTABLE}
    COMMAND ${PYTHON_EXECUTABLE} -m pip install --upgrade pip
    COMMAND ${PYTHON_EXECUTABLE} -m pip install Pillow
    COMMENT "Installing Pillow in virtual environment"
)

# Step 3: Generate font C files
set(GENERATED_FONT_SRC_FILES "")

list(APPEND GENERATED_FONT_SRC_FILES ${TEXT_C_SRC})
add_custom_command(
    OUTPUT ${TEXT_C_SRC}
    DEPENDS ${FONT_SCRIPT} ${TEXT_TTF} ${PIL_PACKAGE_DIR}
    COMMAND ${PYTHON_EXECUTABLE} ${FONT_SCRIPT} --c-name=${TEXT_C_NAME} --height=${TEXT_HEIGHT} ${TEXT_TTF} ${TEXT_C_SRC}
    COMMENT "Generating font file ${TEXT_C_SRC}"
)

list(APPEND GENERATED_FONT_SRC_FILES ${CLOCK_C_SRC})
add_custom_command(
    OUTPUT ${CLOCK_C_SRC}
    DEPENDS ${FONT_SCRIPT} ${CLOCK_TTF} ${PIL_PACKAGE_DIR}
    COMMAND ${PYTHON_EXECUTABLE} ${FONT_SCRIPT} --clock --c-name=${CLOCK_C_NAME} --height=${LCD_HEIGHT} --width=${LCD_WIDTH} ${CLOCK_TTF} ${CLOCK_C_SRC}
    COMMENT "Generating font file ${CLOCK_C_SRC}"
)

# Step 4: Create a custom target to build all fonts
add_custom_target(generate_fonts ALL
    DEPENDS ${GENERATED_FONT_SRC_FILES}
    COMMENT "Generating all fonts"
)

# Export generated font files to parent scope
set(GENERATED_FONT_SRC_FILES ${GENERATED_FONT_SRC_FILES} PARENT_SCOPE)
