# Set variables
set(FONTGEN_DIR ${CMAKE_SOURCE_DIR}/fontgen)
set(FONT_SCRIPT ${FONTGEN_DIR}/create-font-bitmaps.py)
set(FONT_FILE ${FONTGEN_DIR}/Roboto-Medium.ttf) # Default font file, can override
set(FONT_SIZES 12 24 48)
set(FONT_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/lib/Fonts)
set(VENV_DIR ${CMAKE_BINARY_DIR}/fontgen_venv)

# Allow override of font file
set(FONT_FILE_PATH "${FONT_FILE}" CACHE STRING "Path to the TTF font file")

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
foreach(SIZE ${FONT_SIZES})
    set(OUT_FILE ${FONT_OUTPUT_DIR}/RobotoMedium${SIZE}.c)
    list(APPEND GENERATED_FONT_FILES ${OUT_FILE})

    add_custom_command(
        OUTPUT ${OUT_FILE}
        DEPENDS ${FONT_SCRIPT} ${FONT_FILE_PATH} ${VENV_DIR}/.pip_installed
        COMMAND ${VENV_DIR}/bin/python3 ${FONT_SCRIPT} ${FONT_FILE_PATH} ${SIZE} ${OUT_FILE}
        COMMENT "Generating font file ${OUT_FILE}"
    )
endforeach()

# Step 4: Create a custom target to group all font generation
add_custom_target(generate_fonts ALL
    DEPENDS ${GENERATED_FONT_FILES}
    COMMENT "Generating all fonts from ${FONT_FILE_PATH}"
)
