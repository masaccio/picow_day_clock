import re
import subprocess
import sys


def run_tests():
    command = ["build/tests_build/test_picow_day_clock", "--verbose"]

    # ANSI escape codes for coloring text
    GREEN = "\033[92m"
    BOLD_RED = "\033[1;91m"  # '1;' applies the bold attribute
    RESET = "\033[0m"

    # Regex patterns matching exact words using word boundaries (\b)
    ok_pattern = re.compile(r"\bOK\b")
    fail_pattern = re.compile(r"\bFAIL")

    try:
        # Start the process, capturing stdout and stderr merged together
        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,  # Line buffered
        )

        # Read the output line by line as it happens
        for line in process.stdout:
            # Check for matches and apply coloring to the entire line
            if fail_pattern.search(line):
                sys.stdout.write(f"{BOLD_RED}{line}{RESET}")
            elif ok_pattern.search(line):
                sys.stdout.write(f"{GREEN}{line}{RESET}")
            else:
                sys.stdout.write(line)

            sys.stdout.flush()

        # Wait for the process to finish and get the exit code
        return_code = process.wait()
        return return_code

    except FileNotFoundError:
        print(f"{BOLD_RED}Error: Executable file not found. Check the path to 'test_picow_day_clock'.{RESET}")
        return 1
    except Exception as e:
        print(f"{BOLD_RED}An unexpected error occurred: {e}{RESET}")
        return 1


if __name__ == "__main__":
    sys.exit(run_tests())
