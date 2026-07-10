import argparse
import os
import re


def to_c_identifier(name):
    """Converts a filename like 'index.html' to a valid C variable 'index_html'"""
    return re.sub(r"[^0-9a-zA-Z]", "_", os.path.basename(name))


def load_defines(filepath):
    """Parses simple #define macros (e.g., #define XXX 100) from a C header file."""
    defines = {}
    if not os.path.exists(filepath):
        print(f"Warning: Config file '{filepath}' not found.")
        return defines

    with open(filepath, "r", encoding="utf-8") as f:
        for line in f:
            # Matches: #define <KEY> <VALUE> (safely ignores trailing comments)
            match = re.match(r"^\s*#define\s+([A-Za-z0-9_]+)\s+([0-9]+)", line)
            if match:
                defines[match.group(1)] = int(match.group(2))
    return defines


def main():
    parser = argparse.ArgumentParser(description="Convert html into a C structure")
    parser.add_argument("input", help="Input HTML file.")
    parser.add_argument("output", help="Output C file")
    parser.add_argument("--config", help="Input config header file to read #defines from", default="config.h")
    parser.add_argument(
        "--html", help="Optional: Output the populated, un-minified HTML for browser debugging", default=None
    )

    args = parser.parse_args()
    print(f"Generating {args.output}")

    # Load the defines strictly from the config file
    defines = load_defines(args.config)

    # Substitution Logic 1: Strict {DEFINE:XXX} lookup
    def repl_define(m):
        key = m.group(1)
        if key in defines:
            return str(defines[key])
        print(f"Warning: {{DEFINE:{key}}} not found in {args.config}")
        return m.group(0)

    with open(args.input, "r", encoding="utf-8") as f_in:
        raw_content = f_in.read()

    templated_content = re.sub(r"\{DEFINE:([^}]+)\}", repl_define, raw_content)

    if args.html:
        print(f"Exporting debug HTML to {args.html}")
        with open(args.html, "w", encoding="utf-8") as f_debug:
            f_debug.write(templated_content)

    # Minify the templated content for the C byte array
    minified = re.sub(r"/\*.*?\*/", "", templated_content, flags=re.DOTALL)
    lines = [line.strip() for line in minified.splitlines() if line.strip()]
    final_c_content = " ".join(lines)

    with open(args.output, "w", encoding="utf-8") as f_c:
        var_name = to_c_identifier(args.input)
        f_c.write('#include "html_form.h"\n\n')
        f_c.write(f"// Generated from {args.input}\n")
        f_c.write(f"const char {var_name}[] = {{\n    ")

        # Dump as bytes
        bytes_data = final_c_content.encode("utf-8")
        for i, b in enumerate(bytes_data):
            f_c.write(f"0x{b:02x}, ")
            # Wrap lines every 12 bytes
            if (i + 1) % 12 == 0:
                f_c.write("\n    ")

        # Null terminator
        f_c.write("0x00\n};\n")

        f_c.write(f"const unsigned int {var_name}_len = sizeof({var_name}) - 1;\n")


if __name__ == "__main__":
    main()
