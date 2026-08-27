"""
C code generator for endian conversion headers (V4).

Produces an output header that contains ONLY:

1. Endianness detection preprocessor block
2. Enum type definitions (reproduced from the parsed model)
3. Struct type definitions with bitfield groups wrapped in
   ``#if SYSTEM_LITTLE_ENDIAN`` / ``#elif SYSTEM_BIG_ENDIAN``
4. The ``static inline`` endian conversion function
5. The ``static inline`` CSV header-print function
6. The ``static inline`` CSV data-print function

No header guards, no ``#include``, no ``#define`` macros.
"""

from typing import List

from .model import HeaderModel, StructDef, EnumDef, FieldDef
from .bitfield import (
    process_struct_fields,
    BitfieldGroup,
    RegularField,
)
from .traversal import (
    TraversalStep,
    StructBeginMarker,
    StructEndMarker,
    LoopBeginMarker,
    LoopEndMarker,
    ConversionAction,
    PrintHeaderAction,
    DataPrintAction,
)


# =========================================================================
# Display-name normalisation  (_Bool -> bool)
# =========================================================================

_DISPLAY_TYPE = {
    "_Bool": "bool",
}


def _display_type(type_name: str) -> str:
    return _DISPLAY_TYPE.get(type_name, type_name)


# =========================================================================
# Public API
# =========================================================================

def generate_endian_convert_header(
    model: HeaderModel,
    convert_steps: List[TraversalStep],
    print_steps: List[TraversalStep],
    data_print_steps: List[TraversalStep],
    file_prefix: str,
    root_struct_name: str,
) -> str:
    """Generate the complete output header file."""
    lines: List[str] = []

    # -- Endianness detection ------------------------------------------
    lines.extend(_endianness_detection_block())
    lines.append("")

    # -- Includes ------------------------------------------------------
    lines.append("#include <stdio.h>")
    for inc in model.includes:
        if "stdio.h" not in inc:
            lines.append(inc)
    lines.append("")

    # -- Original macro defines ----------------------------------------
    for defn in model.defines:
        lines.append(defn)
    if model.defines:
        lines.append("")

    # -- Enum definitions ----------------------------------------------
    for enum_def in model.enums.values():
        lines.extend(_generate_enum(enum_def))
        lines.append("")

    # -- Struct definitions (with bitfield reordering) -----------------
    for struct_def in model.structs.values():
        lines.extend(_generate_struct(struct_def))
        lines.append("")

    # -- Endian conversion function ------------------------------------
    convert_func = f"{file_prefix}_endian_convert"
    lines.extend(
        _generate_conversion_function(convert_func, root_struct_name,
                                      convert_steps)
    )
    lines.append("")

    # -- Header print function -----------------------------------------
    hprint_func = f"{file_prefix}_header_print"
    lines.extend(
        _generate_header_print_function(hprint_func, print_steps)
    )
    lines.append("")

    # -- Data print function -------------------------------------------
    dprint_func = f"{file_prefix}_data_print"
    lines.extend(
        _generate_data_print_function(dprint_func, root_struct_name,
                                      data_print_steps, convert_func)
    )
    lines.append("")

    return "\n".join(lines)


# =========================================================================
# Endianness detection block
# =========================================================================

def _endianness_detection_block() -> List[str]:
    return [
        "/* Endianness detection */",
        "#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && "
        "(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)",
        "    #define SYSTEM_LITTLE_ENDIAN 1",
        "#elif defined(_WIN32) || defined(_WIN64)",
        "    #define SYSTEM_LITTLE_ENDIAN 1",
        "#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && "
        "(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)",
        "    #define SYSTEM_BIG_ENDIAN 1",
        "#else",
        '    #error "Unknown endianness!"',
        "#endif",
    ]


# =========================================================================
# Enum definition
# =========================================================================

def _generate_enum(enum_def: EnumDef) -> List[str]:
    lines: List[str] = []
    lines.append("typedef enum {")
    for idx, (name, value) in enumerate(enum_def.values):
        comma = "," if idx < len(enum_def.values) - 1 else ""
        lines.append(f"    {name} = {value}{comma}")
    lines.append(f"}} {enum_def.name};")
    return lines


# =========================================================================
# Struct definition  (with bitfield reordering)
# =========================================================================

def _generate_struct(struct_def: StructDef) -> List[str]:
    lines: List[str] = []

    if struct_def.is_packed:
        lines.append("typedef struct __attribute__((packed)) {")
    else:
        lines.append("typedef struct {")

    elements = process_struct_fields(struct_def.fields)

    for elem in elements:
        if isinstance(elem, RegularField):
            lines.append(f"    {_format_field(elem.field)}")

        elif isinstance(elem, BitfieldGroup):
            lines.append("#if defined(SYSTEM_LITTLE_ENDIAN)")
            for fld in reversed(elem.fields):
                lines.append(
                    f"    {elem.dominant_type} {fld.name} : {fld.bit_width};"
                )
            lines.append("#elif defined(SYSTEM_BIG_ENDIAN)")
            for fld in elem.fields:
                lines.append(
                    f"    {elem.dominant_type} {fld.name} : {fld.bit_width};"
                )
            lines.append("#endif")

    lines.append(f"}} {struct_def.name};")
    return lines


def _format_field(field: FieldDef) -> str:
    dtype = _display_type(field.type_name)

    if field.bit_width is not None:
        return f"{dtype} {field.name} : {field.bit_width};"

    if field.is_array:
        size_expr = field.array_size_expr or str(field.array_size)
        return f"{dtype} {field.name}[{size_expr}];"

    return f"{dtype} {field.name};"


# =========================================================================
# Endian conversion function
# =========================================================================

def _generate_conversion_function(
    func_name: str,
    root_struct_name: str,
    steps: List[TraversalStep],
) -> List[str]:
    lines: List[str] = []
    lines.append(
        f"static inline void {func_name}({root_struct_name}* data) {{"
    )

    indent_level = 1

    for step in steps:
        indent = "    " * indent_level

        if isinstance(step, StructBeginMarker):
            lines.append(f"{indent}// BEGIN {step.struct_name}")
        elif isinstance(step, StructEndMarker):
            lines.append(f"{indent}// END {step.struct_name}")
        elif isinstance(step, LoopBeginMarker):
            lines.append(
                f"{indent}for (int {step.index_var} = 0; "
                f"{step.index_var} < {step.limit_expr}; "
                f"{step.index_var}++) {{"
            )
            indent_level += 1
        elif isinstance(step, LoopEndMarker):
            indent_level -= 1
            indent = "    " * indent_level
            lines.append(f"{indent}}}")
        elif isinstance(step, ConversionAction):
            if step.is_enum:
                lines.append(
                    f"{indent}{step.access_path} = "
                    f"({step.enum_type_name}) "
                    f"{step.conversion_func}"
                    f"((uint32_t)({step.access_path}));"
                )
            else:
                lines.append(
                    f"{indent}{step.access_path} = "
                    f"{step.conversion_func}({step.access_path});"
                )

    lines.append("}")
    return lines


# =========================================================================
# Header print function
# =========================================================================

def _generate_header_print_function(
    func_name: str,
    steps: List[TraversalStep],
) -> List[str]:
    lines: List[str] = []
    lines.append(
        f"static inline void {func_name}(FILE *fp) {{"
    )

    lines.append('    fprintf(fp, "DEVICE ID,COMPONENT TYPE,LOG MESSAGE TYPE,TIMESTAMP,LOG PAYLOAD LENGTH,");')

    indent_level = 1

    for step in steps:
        indent = "    " * indent_level

        if isinstance(step, StructBeginMarker):
            lines.append(f"{indent}// BEGIN {step.struct_name}")
        elif isinstance(step, StructEndMarker):
            lines.append(f"{indent}// END {step.struct_name}")
        elif isinstance(step, LoopBeginMarker):
            lines.append(
                f"{indent}for (int {step.index_var} = 0; "
                f"{step.index_var} < {step.limit_expr}; "
                f"{step.index_var}++) {{"
            )
            indent_level += 1
        elif isinstance(step, LoopEndMarker):
            indent_level -= 1
            indent = "    " * indent_level
            lines.append(f"{indent}}}")
        elif isinstance(step, PrintHeaderAction):
            sep = "\\n" if step.is_last else ","
            if step.index_vars:
                args = ", ".join(step.index_vars)
                lines.append(
                    f'{indent}fprintf(fp, "{step.format_string}{sep}", {args});'
                )
            else:
                lines.append(
                    f'{indent}fprintf(fp, "{step.format_string}{sep}");'
                )

    lines.append("}")
    return lines


# =========================================================================
# Data print function
# =========================================================================

def _generate_data_print_function(
    func_name: str,
    root_struct_name: str,
    steps: List[TraversalStep],
    convert_func_name: str = "",
) -> List[str]:
    lines: List[str] = []
    lines.append(
        f"static inline void {func_name}(FILE *fp, {root_struct_name}* data) {{"
    )
    if convert_func_name:
        lines.append(f"    {convert_func_name}(data);")

    indent_level = 1

    for step in steps:
        indent = "    " * indent_level

        if isinstance(step, StructBeginMarker):
            lines.append(f"{indent}// BEGIN {step.struct_name}")
        elif isinstance(step, StructEndMarker):
            lines.append(f"{indent}// END {step.struct_name}")
        elif isinstance(step, LoopBeginMarker):
            lines.append(
                f"{indent}for (int {step.index_var} = 0; "
                f"{step.index_var} < {step.limit_expr}; "
                f"{step.index_var}++) {{"
            )
            indent_level += 1
        elif isinstance(step, LoopEndMarker):
            indent_level -= 1
            indent = "    " * indent_level
            lines.append(f"{indent}}}")
        elif isinstance(step, DataPrintAction):
            sep = "\\n" if step.is_last else ","
            if step.cast_expr:
                value = f"{step.cast_expr}{step.access_path}"
            else:
                value = step.access_path
            lines.append(
                f'{indent}fprintf(fp, "{step.format_spec}{sep}", {value});'
            )

    lines.append("}")
    return lines
