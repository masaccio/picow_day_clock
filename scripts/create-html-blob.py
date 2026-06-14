import argparse
import os
import re


def to_c_identifier(name):
    """Converts a filename like 'index.html' to a valid C variable 'index_html'"""
    return re.sub(r"[^0-9a-zA-Z]", "_", os.path.basename(name))


def main():
    parser = argparse.ArgumentParser(description="Convert html into a C structure")
    parser.add_argument("input", help="Input HTML file.")
    parser.add_argument("output", help="Output C file")

    args = parser.parse_args()
    print(f"Generating {args.output}")

    with open(args.output, "w", encoding="utf-8") as f_c:
        var_name = to_c_identifier(args.input)
        f_c.write(f'#include "html_form.h"\n\n')

        with open(args.input, "r", encoding="utf-8") as f_in:
            raw_content = f_in.read()

        # Minify code: strip comments and leading whitespace
        content = re.sub(r'/\*.*?\*/', '', raw_content, flags=re.DOTALL)
        lines = [line.strip() for line in content.splitlines() if line.strip()]
        content = " ".join(lines)

        f_c.write(f"// Generated from {args.input}\n")
        f_c.write(f"const char {var_name}[] = {{\n    ")

        # Dump as bytes to avoid -Woverlength-strings
        bytes_data = content.encode('utf-8')
        for i, b in enumerate(bytes_data):
            f_c.write(f"0x{b:02x}, ")
            # Wrap lines every 12 bytes so the .c file isn't one infinitely long line
            if (i + 1) % 12 == 0:
                f_c.write("\n    ")
        
        # Null terminator
        f_c.write("0x00\n};\n")
        
        f_c.write(f"const unsigned int {var_name}_len = sizeof({var_name}) - 1;\n")



if __name__ == "__main__":
    main()

