"""
Mock log file generator.

Generates binary .log files simulating PowerPC (big-endian) output,
along with human-readable text and expected CSV files for verification.

PowerPC bitfield packing: MSB-first (the original field order from the
input header is the big-endian order).

All structs are written packed (no padding) regardless of
__attribute__((packed)) — the test headers should declare all structs
as packed for sizeof consistency.
"""

import struct as struct_mod
import random
import os
from typing import List, Tuple, Optional, BinaryIO

from CORE.model import HeaderModel, StructDef, FieldDef
from CORE.bitfield import process_struct_fields, BitfieldGroup, RegularField, TYPE_BITS
from CORE.traversal import _is_reserved


# ---------------------------------------------------------------------------
# Binary pack format for each primitive type (big-endian)
# ---------------------------------------------------------------------------
_PACK_FORMAT = {
    "uint8_t":  ">B",
    "int8_t":   ">b",
    "uint16_t": ">H",
    "int16_t":  ">h",
    "uint32_t": ">I",
    "int32_t":  ">i",
    "uint64_t": ">Q",
    "int64_t":  ">q",
    "bool":     ">B",
    "_Bool":    ">B",
}

# Byte sizes for human-readable output
_TYPE_BYTES = {
    "uint8_t":  1,
    "int8_t":   1,
    "uint16_t": 2,
    "int16_t":  2,
    "uint32_t": 4,
    "int32_t":  4,
    "uint64_t": 8,
    "int64_t":  8,
    "bool":     1,
    "_Bool":    1,
}

# Max values for random generation
_MAX_VALUE = {
    "uint8_t":  255,
    "int8_t":   127,
    "uint16_t": 65535,
    "int16_t":  32767,
    "uint32_t": 2**32 - 1,
    "int32_t":  2**31 - 1,
    "uint64_t": 2**64 - 1,
    "int64_t":  2**63 - 1,
    "bool":     1,
    "_Bool":    1,
}


# =========================================================================
# Public API
# =========================================================================

def generate_mock_log(
    model: HeaderModel,
    struct_sequence: List[Tuple[str, int]],
    seed: int,
    log_path: str,
    txt_path: str,
    csv_path: str,
    preview_count: int = 5,
) -> int:
    """Generate mock .log, .txt, and expected .csv files.

    Args:
        model:            Parsed header model.
        struct_sequence:  List of (struct_name, repeat_count) in order.
                          The *last* entry is the root struct.
        seed:             RNG seed for reproducibility.
        log_path:         Output binary log file path.
        txt_path:         Output human-readable text file path.
        csv_path:         Output expected-values CSV file path.
        preview_count:    Max repetitions to write to the text file.

    Returns:
        Number of root-struct data rows written.
    """
    rng = random.Random(seed)
    root_name = struct_sequence[-1][0]

    all_txt_lines: List[str] = []
    all_csv_rows: List[str] = []
    csv_header_row: Optional[str] = None

    with open(log_path, "wb") as log_fp:
        writer = _MockLogWriter(model, rng, log_fp)

        for struct_name, count in struct_sequence:
            is_root = (struct_name == root_name)

            for i in range(count):
                collect_csv = is_root
                collect_txt = (not is_root) or (i < preview_count)

                csv_vals, txt_lines, csv_headers = writer.write_struct_instance(
                    struct_name,
                    collect_csv=collect_csv,
                    collect_txt=collect_txt,
                )
                
                if collect_csv and csv_header_row is None and csv_headers:
                    csv_header_row = ",".join(csv_headers)

                if collect_txt:
                    all_txt_lines.append(f"--- {struct_name} [{i}] ---")
                    all_txt_lines.extend(txt_lines)
                    all_txt_lines.append("")

                if collect_csv:
                    all_csv_rows.append(",".join(csv_vals))

    # Write text file
    with open(txt_path, "w", newline="\n") as fp:
        fp.write("\n".join(all_txt_lines) + "\n")

    # Write expected CSV (header row + data rows)
    with open(csv_path, "w", newline="\n") as fp:
        if csv_header_row:
            fp.write(csv_header_row + "\n")
        for row in all_csv_rows:
            fp.write(row + "\n")

    return len(all_csv_rows)


# =========================================================================
# Internal writer class
# =========================================================================

class _MockLogWriter:
    """Walks struct definitions, generates random values, writes binary."""

    def __init__(
        self,
        model: HeaderModel,
        rng: random.Random,
        log_fp: BinaryIO,
    ):
        self.model = model
        self.rng = rng
        self.log_fp = log_fp
        self._csv_headers: List[str] = []
        self._csv_values: List[str] = []
        self._txt_lines: List[str] = []

    # ----- public -----

    def write_struct_instance(
        self,
        struct_name: str,
        collect_csv: bool = False,
        collect_txt: bool = False,
    ) -> Tuple[List[str], List[str], List[str]]:
        """Write one struct instance.  Returns (csv_values, txt_lines, csv_headers)."""
        self._csv_headers = []
        self._csv_values = []
        self._txt_lines = []

        struct_def = self.model.structs[struct_name]
        self._write_struct(struct_def, "", collect_csv, collect_txt)

        return list(self._csv_values), list(self._txt_lines), list(self._csv_headers)

    # ----- struct / field walkers -----

    def _write_struct(
        self,
        struct_def: StructDef,
        prefix: str,
        csv: bool,
        txt: bool,
    ):
        elements = process_struct_fields(struct_def.fields)

        for elem in elements:
            if isinstance(elem, RegularField):
                fld = elem.field
                # A lone bitfield emitted as RegularField by process_struct_fields
                if fld.bit_width is not None:
                    self._write_bitfield_group(
                        BitfieldGroup(
                            fields=[fld],
                            unit_bits=TYPE_BITS.get(fld.type_name, 8),
                            dominant_type=fld.type_name,
                        ),
                        prefix, csv, txt,
                    )
                else:
                    self._write_regular(fld, prefix, csv, txt)
            elif isinstance(elem, BitfieldGroup):
                self._write_bitfield_group(elem, prefix, csv, txt)

    def _write_regular(
        self,
        fld: FieldDef,
        prefix: str,
        csv: bool,
        txt: bool,
    ):
        if fld.type_category == "struct":
            if fld.is_array:
                for idx in range(fld.array_size):
                    path = f"{prefix}.{fld.name}[{idx}]" if prefix else f"{fld.name}[{idx}]"
                    nested = self.model.structs.get(fld.type_name)
                    if nested:
                        self._write_struct(nested, path, csv, txt)
            else:
                path = f"{prefix}.{fld.name}" if prefix else fld.name
                nested = self.model.structs.get(fld.type_name)
                if nested:
                    self._write_struct(nested, path, csv, txt)
        else:
            # primitive / enum / bool — possibly array
            if fld.is_array:
                for idx in range(fld.array_size):
                    path = f"{prefix}.{fld.name}[{idx}]" if prefix else f"{fld.name}[{idx}]"
                    self._write_leaf(fld, path, csv, txt)
            else:
                path = f"{prefix}.{fld.name}" if prefix else fld.name
                self._write_leaf(fld, path, csv, txt)

    def _write_leaf(self, fld: FieldDef, path: str, csv: bool, txt: bool):
        """Generate value, write binary, optionally record csv/txt."""
        value = self._gen_value(fld)
        self._write_binary(fld, value)
        if not _is_reserved(fld.name):
            if txt:
                if fld.type_category == "enum":
                    nbytes = 4  # enums are stored as uint32_t
                else:
                    nbytes = _TYPE_BYTES.get(fld.type_name, 4)
                self._txt_lines.append(f"({nbytes} byte) {path} = {value}")
            if csv:
                self._csv_headers.append(path)
                self._csv_values.append(str(value))

    def _write_bitfield_group(
        self,
        group: BitfieldGroup,
        prefix: str,
        csv: bool,
        txt: bool,
    ):
        # Generate values for each bitfield in the group
        bf_values: List[Tuple[FieldDef, int]] = []
        for bf in group.fields:
            max_val = (1 << bf.bit_width) - 1
            val = self.rng.randint(0, max_val)
            bf_values.append((bf, val))

        # Pack in Big-Endian bit order (MSB-first) matching how a PowerPC
        # compiler allocates bitfields. The first bitfield takes the highest bits.
        packed = 0
        current_shift = group.unit_bits
        for bf, val in bf_values:
            current_shift -= bf.bit_width
            packed |= (val & ((1 << bf.bit_width) - 1)) << current_shift

        # Write packed storage unit in Big-Endian byte order
        fmt_map = {8: ">B", 16: ">H", 32: ">I", 64: ">Q"}
        fmt = fmt_map.get(group.unit_bits, ">B")
        self.log_fp.write(struct_mod.pack(fmt, packed))

        # Record non-reserved values
        for bf, val in bf_values:
            if not _is_reserved(bf.name):
                path = f"{prefix}.{bf.name}" if prefix else bf.name
                if txt:
                    self._txt_lines.append(f"({bf.bit_width} bit) {path} = {val}")
                if csv:
                    self._csv_headers.append(path)
                    self._csv_values.append(str(val))

    # ----- value helpers -----

    def _gen_value(self, fld: FieldDef) -> int:
        """Generate a random value appropriate for the field type."""
        if fld.type_category == "enum":
            enum_def = self.model.enums.get(fld.type_name)
            if enum_def and enum_def.values:
                return self.rng.choice(enum_def.values)[1]
            return self.rng.randint(0, 3)

        if fld.type_name in ("bool", "_Bool"):
            return self.rng.randint(0, 1)

        max_val = _MAX_VALUE.get(fld.type_name, 255)
        # Use smaller values for readability
        if max_val > 10000:
            return self.rng.randint(1, 10000)
        return self.rng.randint(0, max_val)

    def _write_binary(self, fld: FieldDef, value: int):
        """Write a single primitive value in big-endian."""
        if fld.type_category == "enum":
            self.log_fp.write(struct_mod.pack(">I", value))
            return
        fmt = _PACK_FORMAT.get(fld.type_name)
        if fmt:
            self.log_fp.write(struct_mod.pack(fmt, value))
        else:
            self.log_fp.write(struct_mod.pack(">I", value))
