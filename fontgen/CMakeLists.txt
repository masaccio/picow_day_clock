# Set variables
set(FONTGEN_DIR ${CMAKE_SOURCE_DIR}/fontgen)
set(FONT_SCRIPT ${FONTGEN_DIR}/create-font-bitmaps.py)
set(FONT_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/generated)
set(VENV_DIR ${CMAKE_BINARY_DIR}/fontgen_venv)

set(TEXT_TTF ${FONTGEN_DIR}/Roboto-Medium.ttf)
set(TEXT_SIZE 20)
set(TEXT_SRC ${FONT_OUTPUT_DIR}/roboto_medium_${TEXT_SIZE}.c)

set(CLOCK_TTF ${FONTGEN_DIR}/BebasNeue-Regular.ttf)
set(CLOCK_SIZE 220)
set(CLOCK_SRC ${FONT_OUTPUT_DIR}/bebasneue_regular_${CLOCK_SIZE}.c)


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
set(GENERATED_FONT_FILES "")

list(APPEND GENERATED_FONT_FILES ${TEXT_SRC})
add_custom_command(
    OUTPUT ${TEXT_SRC}
    DEPENDS ${FONT_SCRIPT} ${TEXT_TTF} ${VENV_DIR}/.pip_installed
    COMMAND ${VENV_DIR}/bin/python3 ${FONT_SCRIPT} --height=${TEXT_SIZE} ${TEXT_TTF} ${TEXT_SRC}
    COMMENT "Generating font file ${TEXT_SRC}"
)

list(APPEND GENERATED_FONT_FILES ${CLOCK_SRC})
add_custom_command(
    OUTPUT ${CLOCK_SRC}
    DEPENDS ${FONT_SCRIPT} ${CLOCK_TTF} ${VENV_DIR}/.pip_installed
    COMMAND ${VENV_DIR}/bin/python3 ${FONT_SCRIPT} --clock --height=${CLOCK_SIZE} ${CLOCK_TTF} ${CLOCK_SRC}
    COMMENT "Generating font file ${CLOCK_SRC}"
)

# Step 4: Create a custom target to group all font generation
add_custom_target(generate_fonts ALL
    DEPENDS ${GENERATED_FONT_FILES}
    COMMENT "Generating all fonts from ${FONT_FILE_PATH}"
)

set(GENERATED_FONTS ${GENERATED_FONT_FILES} PARENT_SCOPE)
