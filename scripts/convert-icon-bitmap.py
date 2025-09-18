import argparse
import textwrap
from typing import TextIO

from PIL import Image


def image_to_bitmap(input_file: TextIO, size: int) -> list[int]:
    """
    Loads an image from a filehandle, resizes to a square size and
    returns a bitmap as a list of 8-bit ints (one pixel per int).
    Args:
        input_file: a readable file handle for the image file
        size: the number of pixels to resize the image to
    Returns:
        A list of ints for the pixels, 8 bits per pixel MSB first.
    """
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
    """
    Convert a list of ints into a C data structure.
    Args:
        data: pixel data as a list of 8-bit ints
        name: the name of the C data structure
    Returns:
        Line-wrapped string of C code.
    """
    hex_data = [f"0x{b:02X}" for b in data]
    wrapped = textwrap.fill(", ".join(hex_data), width=80)
    return f"const uint8_t {name}[{len(data)}] = {{\n{wrapped}\n}};\n"


def main():
    parser = argparse.ArgumentParser(description="Convert images into bitmaps in C")
    parser.add_argument("input", help="Input image file (JPEG/PNG).")
    parser.add_argument("--c-name", required=False, help="C name of the bitmap; default is 'bitmap'.")
    parser.add_argument("--size", type=int, required=True, help="Size of the generated bitmap.")
    parser.add_argument("--output", type=argparse.FileType("w"), required=True, help="Path to the output C file.")

    args = parser.parse_args()

    data = image_to_bitmap(args.input, args.size)
    c_array = format_as_c_array(data, args.c_name or "bitmap")

    args.output.write("#include <stdint.h>\n\n")
    args.output.write(c_array)


if __name__ == "__main__":
    main()
