"""
DFS traversal of struct hierarchies for endian conversion,
CSV header printing, and CSV data printing.

Walks the struct tree depth-first and emits flat sequences of
*TraversalStep* objects that the code generator consumes to
produce C functions.
"""

from dataclasses import dataclass, field
from typing import List, Optional, Tuple, Union

from .model import HeaderModel, FieldDef
from .utils import get_index_variable
from .bitfield import TYPE_BITS


# =========================================================================
# Traversal step types
# =========================================================================

@dataclass
class StructBeginMarker:
    """Marks the beginning of a struct's fields."""
    struct_name: str


@dataclass
class StructEndMarker:
    """Marks the end of a struct's fields."""
    struct_name: str


@dataclass
class LoopBeginMarker:
    """Opening of a ``for`` loop over an array field."""
    index_var: str
    limit_expr: str


@dataclass
class LoopEndMarker:
    """Closing brace of a ``for`` loop."""
    pass


@dataclass
class ConversionAction:
    """A single endian conversion statement."""
    access_path: str
    type_name: str
    conversion_func: str        # "ntohs" | "ntohl" | "ntohll"
    is_enum: bool = False
    enum_type_name: str = ""


@dataclass
class PrintHeaderAction:
    """A single fprintf statement for CSV header printing.

    Attributes:
        format_string: Display path with ``%d`` for loop indices.
        index_vars:    Loop variable names, e.g. ``["i", "j"]``.
        is_last:       True for the very last print action (uses \\n).
    """
    format_string: str
    index_vars: List[str] = field(default_factory=list)
    is_last: bool = False


@dataclass
class DataPrintAction:
    """A single fprintf statement for CSV data value printing.

    Attributes:
        access_path: C expression, e.g. ``"data->sections[i].sectionId"``.
        format_spec: printf format, e.g. ``"%u"`` or ``"%llu"``.
        cast_expr:   Type cast, e.g. ``"(unsigned int)"``.
        is_last:     True for the very last print action (uses \\n).
    """
    access_path: str
    format_spec: str
    cast_expr: str
    is_last: bool = False


# Union of all possible step types
TraversalStep = Union[
    StructBeginMarker,
    StructEndMarker,
    LoopBeginMarker,
    LoopEndMarker,
    ConversionAction,
    PrintHeaderAction,
    DataPrintAction,
]


# =========================================================================
# Conversion mapping
# =========================================================================

ENDIAN_CONVERSIONS = {
    "uint16_t": "ntohs",
    "uint32_t": "ntohl",
    "uint64_t": "ntohll",
    "int16_t": "ntohs",
    "int32_t": "ntohl",
    "int64_t": "ntohll",
}

# Primitives that need no byte-swap
_NO_CONVERSION = frozenset({"uint8_t", "int8_t", "bool", "_Bool"})


# =========================================================================
# Data-print format mapping
# =========================================================================

_DATA_PRINT_FORMAT = {
    "uint8_t":  ("%u",   ""),
    "int8_t":   ("%d",   ""),
    "uint16_t": ("%u",   ""),
    "int16_t":  ("%d",   ""),
    "uint32_t": ("%u",   ""),
    "int32_t":  ("%d",   ""),
    "uint64_t": ("%llu", ""),
    "int64_t":  ("%lld", ""),
    "bool":     ("%u",   "(unsigned int)"),
    "_Bool":    ("%u",   "(unsigned int)"),
}


# =========================================================================
# Reserved field helper
# =========================================================================

def _is_reserved(name: str) -> bool:
    """Return True for fields that should be skipped everywhere."""
    return name.startswith("reserved")


# =========================================================================
# Public API — endian conversion traversal
# =========================================================================

def traverse(model: HeaderModel, root_struct_name: str) -> List[TraversalStep]:
    """DFS traversal for endian conversion.

    Skips bitfield fields and reserved fields.
    """
    steps, _ = _traverse_struct(
        model,
        struct_name=root_struct_name,
        access_prefix="data",
        loop_depth=0,
        is_root=True,
    )
    return steps


# =========================================================================
# Public API — header print traversal
# =========================================================================

def traverse_for_header_print(
    model: HeaderModel, root_struct_name: str,
) -> List[TraversalStep]:
    """DFS traversal for CSV header printing.

    Includes all leaf fields (incl. bitfields) except reserved.
    The last :class:`PrintHeaderAction` has ``is_last = True``.
    """
    steps, _ = _traverse_struct_print(
        model,
        struct_name=root_struct_name,
        display_prefix="",
        active_index_vars=[],
        loop_depth=0,
    )
    for step in reversed(steps):
        if isinstance(step, PrintHeaderAction):
            step.is_last = True
            break
    return steps


# =========================================================================
# Public API — data print traversal
# =========================================================================

def traverse_for_data_print(
    model: HeaderModel, root_struct_name: str,
) -> List[TraversalStep]:
    """DFS traversal for CSV data value printing.

    Includes all leaf fields (incl. bitfields) except reserved.
    The last :class:`DataPrintAction` has ``is_last = True``.
    """
    steps, _ = _traverse_struct_data_print(
        model,
        struct_name=root_struct_name,
        access_prefix="data",
        loop_depth=0,
        is_root=True,
    )
    for step in reversed(steps):
        if isinstance(step, DataPrintAction):
            step.is_last = True
            break
    return steps


# =========================================================================
# Internal — endian conversion
# =========================================================================

def _traverse_struct(
    model: HeaderModel,
    struct_name: str,
    access_prefix: str,
    loop_depth: int,
    is_root: bool = False,
) -> Tuple[List[TraversalStep], int]:

    struct_def = model.structs.get(struct_name)
    if struct_def is None:
        print(f"  WARNING: Struct '{struct_name}' not found in model, skipping.")
        return [], loop_depth

    steps: List[TraversalStep] = []
    steps.append(StructBeginMarker(struct_name))

    for fld in struct_def.fields:
        if fld.bit_width is not None:
            # Full-width bitfields (e.g. uint64_t x : 64) are effectively
            # regular fields and still need endian conversion.
            type_bits = TYPE_BITS.get(fld.type_name, 0)
            if fld.bit_width != type_bits or type_bits == 0:
                continue
        if _is_reserved(fld.name):
            continue

        separator = "->" if is_root else "."
        base_access = f"{access_prefix}{separator}{fld.name}"

        if fld.is_array:
            index_var = get_index_variable(loop_depth)
            loop_depth += 1
            limit = fld.array_size_expr or str(fld.array_size)
            steps.append(LoopBeginMarker(index_var, limit))

            element_access = f"{base_access}[{index_var}]"

            if fld.type_category == "struct":
                sub, loop_depth = _traverse_struct(
                    model, fld.type_name, element_access,
                    loop_depth, is_root=False,
                )
                steps.extend(sub)
            else:
                action = _make_conversion(fld, element_access)
                if action is not None:
                    steps.append(action)

            steps.append(LoopEndMarker())
        else:
            if fld.type_category == "struct":
                sub, loop_depth = _traverse_struct(
                    model, fld.type_name, base_access,
                    loop_depth, is_root=False,
                )
                steps.extend(sub)
            else:
                action = _make_conversion(fld, base_access)
                if action is not None:
                    steps.append(action)

    steps.append(StructEndMarker(struct_name))
    return steps, loop_depth


# =========================================================================
# Internal — header print
# =========================================================================

def _traverse_struct_print(
    model: HeaderModel,
    struct_name: str,
    display_prefix: str,
    active_index_vars: List[str],
    loop_depth: int,
) -> Tuple[List[TraversalStep], int]:

    struct_def = model.structs.get(struct_name)
    if struct_def is None:
        print(f"  WARNING: Struct '{struct_name}' not found in model, skipping.")
        return [], loop_depth

    steps: List[TraversalStep] = []
    steps.append(StructBeginMarker(struct_name))

    for fld in struct_def.fields:
        if _is_reserved(fld.name):
            continue

        if display_prefix:
            field_display = f"{display_prefix}.{fld.name}"
        else:
            field_display = fld.name

        if fld.is_array:
            index_var = get_index_variable(loop_depth)
            loop_depth += 1
            limit = fld.array_size_expr or str(fld.array_size)
            steps.append(LoopBeginMarker(index_var, limit))

            if display_prefix:
                element_display = f"{display_prefix}.{fld.name}[%d]"
            else:
                element_display = f"{fld.name}[%d]"
            new_vars = active_index_vars + [index_var]

            if fld.type_category == "struct":
                sub, loop_depth = _traverse_struct_print(
                    model, fld.type_name,
                    element_display, new_vars,
                    loop_depth,
                )
                steps.extend(sub)
            else:
                steps.append(PrintHeaderAction(
                    format_string=element_display,
                    index_vars=list(new_vars),
                ))

            steps.append(LoopEndMarker())
        else:
            if fld.type_category == "struct":
                sub, loop_depth = _traverse_struct_print(
                    model, fld.type_name,
                    field_display, list(active_index_vars),
                    loop_depth,
                )
                steps.extend(sub)
            else:
                steps.append(PrintHeaderAction(
                    format_string=field_display,
                    index_vars=list(active_index_vars),
                ))

    steps.append(StructEndMarker(struct_name))
    return steps, loop_depth


# =========================================================================
# Internal — data print
# =========================================================================

def _traverse_struct_data_print(
    model: HeaderModel,
    struct_name: str,
    access_prefix: str,
    loop_depth: int,
    is_root: bool = False,
) -> Tuple[List[TraversalStep], int]:

    struct_def = model.structs.get(struct_name)
    if struct_def is None:
        print(f"  WARNING: Struct '{struct_name}' not found in model, skipping.")
        return [], loop_depth

    steps: List[TraversalStep] = []
    steps.append(StructBeginMarker(struct_name))

    for fld in struct_def.fields:
        if _is_reserved(fld.name):
            continue

        separator = "->" if is_root else "."
        base_access = f"{access_prefix}{separator}{fld.name}"

        if fld.is_array:
            index_var = get_index_variable(loop_depth)
            loop_depth += 1
            limit = fld.array_size_expr or str(fld.array_size)
            steps.append(LoopBeginMarker(index_var, limit))

            element_access = f"{base_access}[{index_var}]"

            if fld.type_category == "struct":
                sub, loop_depth = _traverse_struct_data_print(
                    model, fld.type_name, element_access,
                    loop_depth, is_root=False,
                )
                steps.extend(sub)
            else:
                action = _make_data_print(fld, element_access)
                if action is not None:
                    steps.append(action)

            steps.append(LoopEndMarker())
        else:
            if fld.type_category == "struct":
                sub, loop_depth = _traverse_struct_data_print(
                    model, fld.type_name, base_access,
                    loop_depth, is_root=False,
                )
                steps.extend(sub)
            else:
                action = _make_data_print(fld, base_access)
                if action is not None:
                    steps.append(action)

    steps.append(StructEndMarker(struct_name))
    return steps, loop_depth


# =========================================================================
# Helpers
# =========================================================================

def _make_conversion(
    field: FieldDef, access_path: str
) -> Optional[ConversionAction]:
    """Create a *ConversionAction* for a leaf field, or *None*."""

    if field.type_category == "enum":
        return ConversionAction(
            access_path=access_path,
            type_name=field.type_name,
            conversion_func="ntohl",
            is_enum=True,
            enum_type_name=field.type_name,
        )

    if field.type_category == "primitive":
        conv_func = ENDIAN_CONVERSIONS.get(field.type_name)
        if conv_func is not None:
            return ConversionAction(
                access_path=access_path,
                type_name=field.type_name,
                conversion_func=conv_func,
            )
        if field.type_name not in _NO_CONVERSION:
            print(
                f"  WARNING: No endian conversion for "
                f"'{field.type_name}' at {access_path}"
            )
        return None

    if field.type_category == "unknown":
        print(
            f"  WARNING: Skipping unknown type "
            f"'{field.type_name}' at {access_path}"
        )
        return None

    return None


def _make_data_print(
    field: FieldDef, access_path: str,
) -> Optional[DataPrintAction]:
    """Create a *DataPrintAction* for a leaf field, or *None*."""

    if field.type_category == "enum":
        return DataPrintAction(
            access_path=access_path,
            format_spec="%u",
            cast_expr="(unsigned int)",
        )

    fmt = _DATA_PRINT_FORMAT.get(field.type_name)
    if fmt is not None:
        return DataPrintAction(
            access_path=access_path,
            format_spec=fmt[0],
            cast_expr=fmt[1],
        )

    print(
        f"  WARNING: Unknown type '{field.type_name}' for "
        f"data print at {access_path}, defaulting to %u"
    )
    return DataPrintAction(
        access_path=access_path,
        format_spec="%u",
        cast_expr="(unsigned int)",
    )
