# Set variables
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


# Step 1: Create Python virtual environment
add_custom_command(
    OUTPUT ${VENV_DIR}/bin/activate
    COMMAND python3 -m venv ${VENV_DIR}
    COMMENT "Creating Python virtual environment for font generation"
)

# Step 2: Install Pillow inside the venv
add_custom_command(
    OUTPUT ${VENV_DIR}/.pip_installed
    DEPENDS ${VENV_DIR}/bin/activate
    COMMAND ${VENV_DIR}/bin/python3 -m pip install Pillow
    COMMAND ${CMAKE_COMMAND} -E touch ${VENV_DIR}/.pip_installed
    COMMENT "Installing Pillow in virtual environment"
)

# Step 3: Generate font C files for each size
set(GENERATED_FONT_SRC_FILES "")

list(APPEND GENERATED_FONT_SRC_FILES ${TEXT_C_SRC})
add_custom_command(
    OUTPUT ${TEXT_C_SRC}
    DEPENDS ${FONT_SCRIPT} ${TEXT_TTF} ${VENV_DIR}/.pip_installed
    COMMAND ${VENV_DIR}/bin/python3 ${FONT_SCRIPT} --c-name=${TEXT_C_NAME} --height=${TEXT_HEIGHT} ${TEXT_TTF} ${TEXT_C_SRC}
    COMMENT "Generating font file ${TEXT_C_SRC}"
)

list(APPEND GENERATED_FONT_SRC_FILES ${CLOCK_C_SRC})
add_custom_command(
    OUTPUT ${CLOCK_C_SRC}
    DEPENDS ${FONT_SCRIPT} ${CLOCK_TTF} ${VENV_DIR}/.pip_installed
    COMMAND ${VENV_DIR}/bin/python3 ${FONT_SCRIPT} --clock --c-name=${CLOCK_C_NAME} --height=${LCD_HEIGHT} --width=${LCD_WIDTH} ${CLOCK_TTF} ${CLOCK_C_SRC}
    COMMENT "Generating font file ${CLOCK_C_SRC}"
)

# Step 4: Create a custom target to group all font generation
add_custom_target(generate_fonts ALL
    DEPENDS ${GENERATED_FONT_SRC_FILES}
    COMMENT "Generating all fonts from ${FONT_FILE_PATH}"
)

set(GENERATED_FONT_SRC_FILES ${GENERATED_FONT_SRC_FILES} PARENT_SCOPE)
