import argparse
import textwrap
from typing import TextIO

from PIL import Image


def image_to_bitmap(input_file: TextIO, size: int) -> list[int]:
    img = Image.open(input_file).convert("1")
    img = img.resize((size, size), Image.LANCZOS)
    img = img.point(lambda x: 0 if x > 128 else 1, "1")
    pixels = list(img.getdata())

    data = []
    for i in range(0, len(pixels), 8):
        byte = 0
        for bit in range(8):
            if i + bit < len(pixels):
                byte |= (pixels[i + bit] & 1) << (7 - bit)  # MSB first
        data.append(byte)

    return data


def format_as_c_array(data: list[int], name: str) -> str:
    hex_data = [f"0x{b:02X}" for b in data]
    wrapped = textwrap.fill(", ".join(hex_data), width=80)
    return f"const unsigned char {name}[] = {{\n{wrapped}\n}};\n"


def main():
    parser = argparse.ArgumentParser(description="Convert images into bitmaps in C")
    parser.add_argument("input", help="Input image file (JPEG/PNG).")
    parser.add_argument("--c-name", required=False, help="C name of the bitmap; default is 'bitmap'.")
    parser.add_argument("--size", type=int, required=True, help="Size of the generated bitmap.")
    parser.add_argument("--output", type=argparse.FileType("w"), required=True, help="Path to the output C file.")

    args = parser.parse_args()

    data = image_to_bitmap(args.input, args.size)
    c_array = format_as_c_array(data, args.c_name or "bitmap")

    args.output.write(f"// Monochrome {args.size}x{args.size} bitmap, {len(data)} bytes\n")
    args.output.write(c_array)


if __name__ == "__main__":
    main()
