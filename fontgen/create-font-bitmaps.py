import sys
import math
from os.path import basename, splitext, dirname
import os
from PIL import Image, ImageDraw, ImageFont  # pyright: ignore[reportMissingImports]


def generate_variable_width_font(ttf_path: str, char_height: int, target: str) -> None:
    font_name: str = basename(splitext(ttf_path)[0]).replace("-", "_").replace(" ", "_")
    font: ImageFont.FreeTypeFont = ImageFont.truetype(ttf_path, char_height)
    chars: list[str] = [chr(i) for i in range(32, 127)]  # Printable ASCII range

    output_height = char_height
    glyph_entries = []
    table_name = f"{font_name}{char_height}_Table"
    font_struct_name = f"{font_name}{char_height}"

    license_path = os.path.join(dirname(dirname(__file__)), "LICENSE")
    license_comment = ""
    try:
        with open(license_path, "r") as lf:
            license_lines = lf.readlines()
        license_comment = (
            "/*\n"
            + "".join([" * " + line.rstrip() + "\n" for line in license_lines])
            + "*/\n\n"
        )
    except Exception:
        license_comment = "/* LICENSE file not found */\n\n"

    with open(target, "w") as fh:
        fh.write(license_comment)
        fh.write('#include "fonts.h"\n\n')

        for char in chars:
            bbox = font.getbbox(char)
            glyph_width = bbox[2] - bbox[0]
            glyph_height = bbox[3] - bbox[1]
            y_offset = max((output_height - glyph_height) // 2, 0)

            image = Image.new("1", (glyph_width, output_height), 0)
            draw = ImageDraw.Draw(image)
            draw.text((-bbox[0], y_offset - bbox[1]), char, font=font, fill=1)

            pixels = image.load()
            bytes_per_row = math.ceil(glyph_width / 8)

            glyph_array_name = f"{font_name}{char_height}_{ord(char)}"

            fh.write(f"static const uint8_t {glyph_array_name}[] = {{\n")
            for y in range(output_height):
                comment = ""
                for byte_index in range(bytes_per_row):
                    byte = 0
                    for bit in range(8):
                        x = byte_index * 8 + bit
                        if x < glyph_width and pixels[x, y] != 0:
                            byte |= 1 << (7 - bit)
                            comment += "#"
                        elif x < glyph_width:
                            comment += " "
                    fh.write(f"  0x{byte:02X}, // {comment}\n")
            fh.write("};\n\n")

            glyph_entries.append((glyph_width, glyph_array_name))

        fh.write(f"static const vFONTENTRY {table_name}[] = {{\n")
        for width, glyph_array_name in glyph_entries:
            fh.write(f"  {{ {width}, {glyph_array_name} }},\n")
        fh.write("};\n\n")

        fh.write(f"vFONT {font_struct_name} = {{\n")
        fh.write(f"  {table_name},\n")
        fh.write(f"  {output_height}\n")
        fh.write("};\n")


def main() -> None:
    if len(sys.argv) != 4:
        print("Usage: python stm32_fontgen.py path/to/font.ttf height path/to/output.c")
        sys.exit(1)

    ttf_path: str = sys.argv[1]
    height: int = int(sys.argv[2])
    target: str = sys.argv[3]
    generate_variable_width_font(ttf_path, height, target)


if __name__ == "__main__":
    main()
    main()
