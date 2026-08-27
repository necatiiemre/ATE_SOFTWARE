"""
TEST_RUN.py - All-in-one test pipeline.

Runs all steps (header gen, mock log, e2e test) in a single command.
Intermediate results are saved per-step for debugging.
Final outputs are collected in OUTPUT/<STEM>/.

Usage:
    python TEST_RUN.py INPUT/SENSOR_TEST.h --structs "FileHeader:1,DeviceInfo:1,SensorReading:3"

Output:
    STEPS/1_HEADER_GEN/<STEM>/          (intermediate)
    STEPS/2_MOCK_LOG/<STEM>/            (intermediate)
    STEPS/3_E2E_TEST/<STEM>/            (intermediate)
    OUTPUT/<STEM>/                      (final results)
        <STEM>_PARSED.h
        <STEM>.log
        <STEM>_EXPECTED.csv
        <STEM>_OUTPUT.csv
        <STEM>_MAIN.c
        <STEM>_MAIN.exe
"""

import sys
import os
import argparse
import subprocess
import shutil

# ---------------------------------------------------------------------------
# Path setup
# ---------------------------------------------------------------------------
_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _SCRIPT_DIR)

_LIBS_DIR = os.path.join(_SCRIPT_DIR, "LIBS")
if os.path.isdir(_LIBS_DIR):
    sys.path.insert(0, _LIBS_DIR)

from CORE.parser import HeaderParser
from CORE.traversal import traverse, traverse_for_header_print, traverse_for_data_print
from CORE.generator import generate_endian_convert_header
from CORE.utils import get_file_prefix
from MODULES.MOCK_GEN import generate_mock_log
from MODULES.MAIN_GEN import generate_test_main


SEP = "=" * 60


def parse_structs_arg(structs_str: str) -> list:
    sequence = []
    for part in structs_str.split(","):
        part = part.strip()
        if not part:
            continue
        if ":" in part:
            name, count = part.split(":", 1)
            sequence.append((name.strip(), int(count.strip())))
        else:
            sequence.append((part.strip(), 1))
    return sequence


def main() -> int:
    parser = argparse.ArgumentParser(
        description="All-in-one test pipeline: parse, mock, compile, run, compare.",
    )
    parser.add_argument("input", help="Path to the input C header file.")
    parser.add_argument("--structs", required=True,
                        help="Struct sequence (e.g., 'FileHeader:1,DeviceInfo:1,SensorReading:3').")
    parser.add_argument("--seed", type=int, default=42,
                        help="Random seed (default: 42).")

    args = parser.parse_args()

    if not os.path.isfile(args.input):
        print(f"ERROR: Input file not found: {args.input}")
        return 1

    struct_seq = parse_structs_arg(args.structs)
    if not struct_seq:
        print("ERROR: --structs cannot be empty.")
        return 1

    root_struct = struct_seq[-1][0]

    # Derive STEM from input filename (uppercase)
    stem_raw = os.path.splitext(os.path.basename(args.input))[0]
    STEM = stem_raw.upper()
    file_prefix = stem_raw.lower()

    # Directory paths
    output_dir = os.path.join(_SCRIPT_DIR, "OUTPUT", STEM)
    os.makedirs(output_dir, exist_ok=True)

    print(SEP)
    print(f"  TEST_RUN: {STEM}")
    print(f"  Root struct: {root_struct}")
    print(f"  Struct sequence: {struct_seq}")
    print(SEP)

    # ================================================================
    # STEP 1: Header Generator
    # ================================================================
    print(f"\n[STEP 1/3] Generating {STEM}_PARSED.h ...")

    header_parser = HeaderParser(args.input)
    model = header_parser.parse()

    if root_struct not in model.structs:
        print(f"ERROR: Root struct '{root_struct}' not found.")
        return 1

    model.root_struct_name = root_struct

    steps = traverse(model, root_struct)
    psteps = traverse_for_header_print(model, root_struct)
    dpsteps = traverse_for_data_print(model, root_struct)

    parsed_code = generate_endian_convert_header(
        model, steps, psteps, dpsteps, file_prefix, root_struct,
    )

    parsed_name = f"{STEM}_PARSED.h"
    parsed_path = os.path.join(output_dir, parsed_name)
    with open(parsed_path, "w", newline="\n") as fh:
        fh.write(parsed_code)
    print(f"  -> {parsed_path}")

    # ================================================================
    # STEP 2: Mock Log Generator
    # ================================================================
    print(f"\n[STEP 2/3] Generating {STEM}.log + {STEM}_EXPECTED.csv ...")

    log_path = os.path.join(output_dir, f"{STEM}.log")
    txt_path = os.path.join(output_dir, f"{STEM}_EXPECTED_RESULTS.txt")
    csv_path = os.path.join(output_dir, f"{STEM}_EXPECTED.csv")

    n_rows = generate_mock_log(
        model, struct_seq, args.seed,
        log_path, txt_path, csv_path,
        preview_count=5,
    )
    print(f"  -> {log_path}")
    print(f"  -> {csv_path}  ({n_rows} data rows)")
    print(f"  -> {txt_path}")

    # ================================================================
    # STEP 3: E2E Test (generate C, compile, run)
    # ================================================================
    print(f"\n[STEP 3/3] Generating {STEM}_MAIN.c, compiling, running ...")

    # Generate test_main.c
    main_c = generate_test_main(
        parsed_header_name=parsed_name,
        log_filename=f"{STEM}.log",
        csv_filename=f"{STEM}_OUTPUT.csv",
        struct_sequence=struct_seq,
        file_prefix=file_prefix,
    )
    main_c_path = os.path.join(output_dir, f"{STEM}_MAIN.c")
    with open(main_c_path, "w", newline="\n") as fh:
        fh.write(main_c)
    print(f"  -> {main_c_path}")

    # Compile
    exe_path = os.path.join(output_dir, f"{STEM}_MAIN.exe")
    compile_cmd = ["gcc", "-o", exe_path, main_c_path, "-Wall", "-Wno-unused-function"]
    result = subprocess.run(compile_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  FAIL: gcc error:\n{result.stderr}")
        return 1
    print(f"  -> {exe_path}  (compiled OK)")

    # Run
    result = subprocess.run([exe_path], capture_output=True, text=True, cwd=output_dir)
    if result.returncode != 0:
        print(f"  FAIL: runtime error:\n{result.stdout}\n{result.stderr}")
        return 1
    print(f"  {result.stdout.strip()}")

    output_csv_path = os.path.join(output_dir, f"{STEM}_OUTPUT.csv")

    # ================================================================
    # COMPARE
    # ================================================================
    print(f"\n{SEP}")
    print("  COMPARING OUTPUT vs EXPECTED")
    print(SEP)

    with open(csv_path, "r") as f:
        expected = f.read().strip()
    with open(output_csv_path, "r") as f:
        actual = f.read().strip()

    if expected == actual:
        print(f"\n  SUCCESS: {STEM}_OUTPUT.csv matches {STEM}_EXPECTED.csv!")
        print(f"\n{SEP}")
        return 0
    else:
        print(f"\n  FAIL: {STEM}_OUTPUT.csv differs from {STEM}_EXPECTED.csv!")
        exp_lines = expected.splitlines()
        act_lines = actual.splitlines()
        for i, (e, a) in enumerate(zip(exp_lines, act_lines)):
            if e != a:
                print(f"\n  First diff at line {i+1}:")
                ef = e.split(",")
                af = a.split(",")
                for j, (ev, av) in enumerate(zip(ef, af)):
                    if ev != av:
                        print(f"    Col {j}: expected='{ev}', actual='{av}'")
                        if j > 4:
                            print(f"    ... (truncated)")
                            break
                break
        print(f"\n{SEP}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
