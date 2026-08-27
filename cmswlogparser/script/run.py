"""
Entry point for the C Header Endian Converter.

Usage:
    python run.py <input.h> --root <RootStructName> [-o output.h] [-v]

Example:
    python run.py log_test.h --root TelemetryLog
    python run.py log_test.h --root TelemetryLog -o output/converted.h -v
"""

import sys
import os
import argparse

# ---------------------------------------------------------------------------
# Portable library setup — add libs/ to sys.path before any project imports
# ---------------------------------------------------------------------------
_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
_LIBS_DIR = os.path.join(_SCRIPT_DIR, "libs")

if os.path.isdir(_LIBS_DIR):
    sys.path.insert(0, _LIBS_DIR)

# ---------------------------------------------------------------------------
# Project imports (after path setup)
# ---------------------------------------------------------------------------
from modules.parser import HeaderParser
from modules.traversal import (
    traverse,
    traverse_for_header_print,
    traverse_for_data_print,
)
from modules.generator import generate_endian_convert_header
from modules.utils import get_file_prefix


def main() -> int:
    parser = argparse.ArgumentParser(
        description="C Header Endian Converter — "
                    "Generate endian conversion functions from C header structs.",
    )
    parser.add_argument(
        "input",
        help="Path to the input C header file.",
    )
    parser.add_argument(
        "--root",
        required=True,
        help="Name of the root struct (required). "
             "The generated function receives a pointer to this type.",
    )
    parser.add_argument(
        "-o", "--output",
        help="Output header file path. "
             "Default: <input_stem>_endian_convert.h in the same directory.",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Print the parsed model summary.",
    )

    args = parser.parse_args()

    # -- Validate input ------------------------------------------------
    if not os.path.isfile(args.input):
        print(f"ERROR: Input file not found: {args.input}")
        return 1

    # -- Derive output path --------------------------------------------
    file_prefix = get_file_prefix(args.input)  # lowercase for functions
    stem_original = os.path.splitext(os.path.basename(args.input))[0]
    if args.output:
        output_path = args.output
    else:
        input_dir = os.path.dirname(os.path.abspath(args.input))
        output_path = os.path.join(
            input_dir, f"{stem_original}_PARSED.h"
        )

    # -- Step 1: Parse -------------------------------------------------
    print(f"[1/5] Parsing: {args.input}")
    header_parser = HeaderParser(args.input)
    model = header_parser.parse()

    if args.verbose:
        print()
        print(model.summary())
        print()

    # Validate root struct
    if args.root not in model.structs:
        print(f"ERROR: Root struct '{args.root}' not found in header.")
        print(f"  Available structs: {', '.join(model.structs.keys())}")
        return 1

    model.root_struct_name = args.root

    # -- Step 2: Traverse (endian convert) -----------------------------
    print(f"[2/5] Traversing for endian conversion...")
    steps = traverse(model, args.root)

    # -- Step 3: Traverse (header print) -------------------------------
    print(f"[3/5] Traversing for header print...")
    print_steps = traverse_for_header_print(model, args.root)

    # -- Step 4: Traverse (data print) ---------------------------------
    print(f"[4/5] Traversing for data print...")
    data_print_steps = traverse_for_data_print(model, args.root)

    # -- Step 5: Generate ----------------------------------------------
    print(f"[5/5] Generating output header...")
    output_content = generate_endian_convert_header(
        model, steps, print_steps, data_print_steps,
        file_prefix, args.root,
    )

    # -- Write output --------------------------------------------------
    out_dir = os.path.dirname(os.path.abspath(output_path))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(output_path, "w", newline="\n") as fh:
        fh.write(output_content)

    print(f"Output written to: {output_path}")
    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
