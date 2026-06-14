import argparse
import os
import re


def to_c_identifier(name):
    """Converts a filename like 'index.html' to a valid C variable 'index_html'"""
    return re.sub(r"[^0-9a-zA-Z]", "_", os.path.basename(name))


def main():
    parser = argparse.ArgumentParser(description="Convert html into a C structure")
    parser.add_argument("input", help="Input HTML file.")
    parser.add_argument("output", help="Output C file (header replaces .c)")

    args = parser.parse_args()

    c_filename = args.output
    h_filename = args.output.replace(".c", ".h")

    print(f"Generating {c_filename} and {h_filename}...")

    with open(c_filename, "w", encoding="utf-8") as f_c, open(h_filename, "w", encoding="utf-8") as f_h:
        f_h.write("#pragma once\n\n")
        f_c.write(f'#include "{h_filename}"\n\n')

        var_name = to_c_identifier(args.input)

        with open(args.input, "r", encoding="utf-8") as f_in:
            content = f_in.read()

        f_h.write(f"extern const char {var_name}[];\n")
        f_h.write(f"extern const unsigned int {var_name}_len;\n\n")

        f_c.write(f"const char {var_name}[] =\n")

        # Process line by line so the C code is actually readable
        for line in content.splitlines(True):
            # Escape backslashes first, then quotes, then newlines
            escaped = line.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
            f_c.write(f'    "{escaped}"\n')

        f_c.write(";\n")

        f_c.write(f"const unsigned int {var_name}_len = sizeof({var_name}) - 1;\n\n")

        print(f" -> Processed {args.input} into '{var_name}'")

    print("Done!")


if __name__ == "__main__":
    main()
