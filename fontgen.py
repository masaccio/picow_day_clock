import sys
import math
from PIL import Image, ImageDraw, ImageFont


def generate_variable_width_font(ttf_path, char_height, font_name="Roboto_Font"):
    font = ImageFont.truetype(ttf_path, char_height)
    chars = [chr(i) for i in range(32, 127)]  # Printable ASCII

    output_height = char_height
    glyph_entries = []
    table_name = f"{font_name}{char_height}_Table"
    font_struct_name = f"{font_name}{char_height}"

    print('#include "fonts.h"\n')

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

        print(f"static const uint8_t {glyph_array_name}[] = {{")
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
                print(f"  0x{byte:02X}, // {comment}")
        print("};\n")

        glyph_entries.append((glyph_width, glyph_array_name))

    print(f"static const vFONTENTRY {table_name}[] = {{")
    for width, glyph_array_name in glyph_entries:
        print(f"  {{ {width}, {glyph_array_name} }},")
    print("};\n")

    print(f"vFONT {font_struct_name} = {{")
    print(f"  {table_name},")
    print(f"  {output_height}")
    print("};")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python stm32_fontgen.py path/to/font.ttf height")
        sys.exit(1)

    ttf_path = sys.argv[1]
    height = int(sys.argv[2])
    generate_variable_width_font(ttf_path, height)
