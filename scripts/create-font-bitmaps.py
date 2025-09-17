import argparse
import math
import os
from os.path import basename, dirname, splitext
from typing import TextIO

from PIL import Image, ImageDraw, ImageFont  # pyright: ignore[reportMissingImports]

DAYS_OF_WEEK = ["MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"]
CLOCK_CHAR_RANGE = sorted(set("".join(DAYS_OF_WEEK))) + [str(x) for x in range(0, 10)]
ASCII_CHAR_RANGE = [chr(i) for i in range(32, 127)]


def get_commented_license(license_path: str) -> str:
    """
    Reads a license file and returns its contents as a C-style comment block.
    Args:
        license_path: Path to the license file.
    Returns:
        License text wrapped in C comment syntax.
    """
    with open(license_path, "r") as lf:
        license_lines = lf.readlines()
    return "/*\n" + "".join([" * " + line.rstrip() + "\n" for line in license_lines]) + "*/\n\n"


def ascii_c_name(char: str) -> str:
    """
    Maps a character to a valid C identifier name for use in arrays.
    Args:
        char: The character to map.
    Returns:
        A string suitable for use as a C identifier.
    """
    special_map = {
        " ": "space",
        "!": "excl",
        '"': "quote",
        "#": "hash",
        "$": "dollar",
        "%": "percent",
        "&": "amp",
        "'": "apos",
        "(": "lparen",
        ")": "rparen",
        "*": "asterisk",
        "+": "plus",
        ",": "comma",
        "-": "minus",
        ".": "dot",
        "/": "slash",
        ":": "colon",
        ";": "semi",
        "<": "lt",
        "=": "eq",
        ">": "gt",
        "?": "qmark",
        "@": "at",
        "[": "lbrack",
        "\\": "bslash",
        "]": "rbrack",
        "^": "caret",
        "_": "underscore",
        "`": "backtick",
        "{": "lbrace",
        "|": "pipe",
        "}": "rbrace",
        "~": "tilde",
    }
    if char.isalnum():
        return char
    return special_map[char]


def max_font_size(font_filename: str, target_height: int) -> int:
    """
    Determines the maximum font size that fits within the target height.
    Args:
        font_filename: Path to the TTF font file.
        target_height: Desired maximum height in pixels.
    Returns:
        The largest font size that fits within the target height.
    """
    """Hack: determine the font size needed to achieve a target height of 12 pixels."""
    max_font_size = int(target_height / 2)
    font_size = max_font_size
    while True:
        font = ImageFont.truetype(font_filename, font_size)
        max_y = max(font.getbbox(char)[3] for char in ASCII_CHAR_RANGE)
        # Allow for case where a single step in font size results in a height that is too large
        if max_y <= target_height:
            max_font_size = font_size
        if max_y >= target_height:
            break
        font_size += 1
    return max_font_size


def generate_variable_width_font(
    font_filename: str, char_height: int, char_width: int, c_name: str, target: TextIO, clock: bool
) -> None:
    """
    Generates a variable-width bitmap font table from a TTF font and writes it to a C file.
    Args:
        font_filename: Path to the TTF font file.
        char_height: Maximum height of each glyph in pixels.
        char_width: Maximum width of each glyph in pixels (unused).
        c_name: C identifier name for the font.
        target: Output file object to write the C code.
        clock: If True, only generate clock characters.
    Returns:
        None
    """
    font_size = max_font_size(font_filename, char_height)
    font = ImageFont.truetype(font_filename, font_size)

    glyph_entries = []
    if c_name is not None:
        table_name = f"{c_name}_table"
        font_struct_name = f"{c_name}"
    else:
        c_name = basename(splitext(font_filename)[0]).replace("-", "_").replace(" ", "_").lower()
        table_name = f"{c_name}_{char_height}_table"
        font_struct_name = f"{c_name}_{char_height}"

    target.write(get_commented_license(os.path.join(dirname(dirname(__file__)), "LICENSE")))
    target.write('#include "bitmap.h"\n\n')

    min_x = min(font.getbbox(char)[0] for char in ASCII_CHAR_RANGE)
    max_x = max(font.getbbox(char)[2] for char in ASCII_CHAR_RANGE)
    max_width = max_x - min_x
    max_height = max(font.getbbox(char)[3] for char in ASCII_CHAR_RANGE)

    bytes_per_row = math.ceil(max_width / 8)

    if max_height > char_height:
        raise ValueError(f"Font height {max_height} exceeds output height {char_height}.")

    for char in ASCII_CHAR_RANGE:
        if clock and char not in CLOCK_CHAR_RANGE:
            glyph_entries.append((-1, "(const uint8_t *)0"))
            continue

        image = Image.new(
            "1",  # 1-bit pixels, black and white
            (bytes_per_row * 8, char_height),
        )
        draw = ImageDraw.Draw(image)
        bbox = font.getbbox(char)
        # Override left offset as we cannot deal with negative offsets (e.g. 'j')
        x_offset = 0 if bbox[0] < 0 else bbox[0]
        draw.text((x_offset, 0), char, font=font, fill=1)
        pixels = image.load()

        glyph_array_name = f"{c_name}_{char_height}_" + ascii_c_name(char)
        target.write(f"static const uint8_t {glyph_array_name}[] = {{\n")
        for y in range(char_height):
            line_bytes = []
            comment = ""
            for byte_index in range(bytes_per_row):
                byte = 0
                for bit in range(8):
                    x = byte_index * 8 + bit
                    if pixels[x, y]:
                        byte |= 1 << (7 - bit)
                        comment += "#"
                    else:
                        comment += " "
                line_bytes.append(byte)
            target.write("  " + ", ".join(f"0x{b:02X}" for b in line_bytes) + f", // {comment}\n")
        target.write("};\n\n")

        glyph_width = bbox[2] - bbox[0]
        glyph_entries.append((glyph_width, glyph_array_name))

    target.write(f"static const font_glyph_t {table_name}[] = {{\n")
    for width, glyph_array_name in glyph_entries:
        target.write(f"  {{ {width}, {glyph_array_name} }},\n")
    target.write("};\n\n")

    target.write(f"font_t {font_struct_name} = {{\n")
    target.write(f"  {table_name},\n")
    target.write(f"  {bytes_per_row},\n")
    target.write(f"  {char_height}\n")
    target.write("};\n")


def main() -> None:
    """
    Parses command-line arguments and generates the font table.
    """
    parser = argparse.ArgumentParser(description="Generate bitmap font tables from a TTF font.")

    parser.add_argument("font", help="Path to the TTF font file.")

    parser.add_argument("output", type=argparse.FileType("w"), help="Path to the output C file.")

    parser.add_argument("--height", type=int, required=True, help="Max height of the font in pixels.")

    parser.add_argument("--width", type=int, required=False, help="Max width of the font in pixels.")

    parser.add_argument("--c-name", required=False, help="C name of the font; default is derived from filename.")

    parser.add_argument("--clock", action="store_true", help="Only generate clock characters.")

    args = parser.parse_args()

    generate_variable_width_font(
        font_filename=args.font,
        char_height=args.height,
        char_width=args.width,
        target=args.output,
        c_name=args.c_name,
        clock=args.clock,
    )


if __name__ == "__main__":
    # execute only if run as a script
    main()
