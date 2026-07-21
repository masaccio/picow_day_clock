import argparse
import os
import re


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
    parser.add_argument("output", help="Output C file.")
    parser.add_argument("--header", help="Output Header file.", default="html_form.h")
    parser.add_argument("--config", help="Input config header file to read #defines from", default="config.h")
    parser.add_argument(
        "--html", help="Optional: Output the populated, un-minified HTML for browser debugging", default=None
    )

    args = parser.parse_args()
    print(f"Generating {args.output} and {args.header}")

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

    bytes_data = final_c_content.encode("utf-8")
    template_length = len(bytes_data)
    var_name = "html_form_template"

    # Write C File
    with open(args.output, "w", encoding="utf-8") as f_c:
        f_c.write(f'#include "{os.path.basename(args.header)}"\n\n')
        f_c.write(f"// Generated from {args.input}\n")
        f_c.write(f"const char {var_name}[] = {{\n    ")

        # Dump as bytes
        for i, b in enumerate(bytes_data):
            f_c.write(f"0x{b:02x}, ")
            # Wrap lines every 12 bytes
            if (i + 1) % 12 == 0:
                f_c.write("\n    ")

        # Null terminator
        f_c.write("0x00\n};\n")

    # Write Header File
    with open(args.header, "w", encoding="utf-8") as f_h:
        f_h.write("#pragma once\n\n")
        f_h.write('#include "config.h"\n\n')
        f_h.write(f"extern const char {var_name}[];\n\n")
        f_h.write(f"#define HTML_FORM_TEMPLATE_LENGTH {template_length}\n")
        f_h.write(
            "#define HTML_FORM_MAX_LENGTH HTML_FORM_TEMPLATE_LENGTH + WIFI_SSID_MAX_LEN + WIFI_PASSWORD_MAX_LEN + HOSTNAME_MAX_LEN\n"
        )


if __name__ == "__main__":
    main()
