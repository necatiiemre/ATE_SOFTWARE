"""
Bitfield processing for struct definitions.

Groups consecutive bitfield declarations by their storage unit,
normalises mixed types to the dominant (widest) type within each
group, and provides helpers for generating the ``#if`` / ``#elif``
conditional-compilation blocks that reorder bits for little-endian
vs big-endian targets.
"""

from dataclasses import dataclass, field
from typing import List, Union

from .model import FieldDef


# =========================================================================
# Type-size lookup
# =========================================================================

TYPE_BITS = {
    "uint8_t": 8,  "int8_t": 8,  "_Bool": 8, "bool": 8,
    "uint16_t": 16, "int16_t": 16,
    "uint32_t": 32, "int32_t": 32,
    "uint64_t": 64, "int64_t": 64,
}

BITS_TO_UNSIGNED = {
    8:  "uint8_t",
    16: "uint16_t",
    32: "uint32_t",
    64: "uint64_t",
}


# =========================================================================
# Result types
# =========================================================================

@dataclass
class BitfieldGroup:
    """Two or more consecutive bitfields that share a storage unit.

    The generator wraps these in ``#if SYSTEM_LITTLE_ENDIAN`` (reversed)
    and ``#elif SYSTEM_BIG_ENDIAN`` (original order).
    """
    fields: List[FieldDef]
    unit_bits: int          # 8, 16, 32, or 64
    dominant_type: str      # e.g. "uint16_t"


@dataclass
class RegularField:
    """A non-bitfield field, or a single-member bitfield group
    (no reordering needed)."""
    field: FieldDef


# Union of all element types within a processed struct
StructElement = Union[BitfieldGroup, RegularField]


# =========================================================================
# Public API
# =========================================================================

def process_struct_fields(fields: List[FieldDef]) -> List[StructElement]:
    """Partition a struct's fields into regular fields and bitfield groups.

    Consecutive bitfield declarations are accumulated into the same
    group until the storage unit is full or a non-bitfield / incompatible
    field is encountered.  Single-member groups are emitted as
    :class:`RegularField` (no ``#if`` wrapper needed).
    """
    elements: List[StructElement] = []

    # -- accumulator state --
    group: List[FieldDef] = []
    group_bits = 0
    group_unit = 0          # unit size in bits

    def _close_group() -> None:
        nonlocal group, group_bits, group_unit
        if not group:
            return
        if len(group) == 1:
            elements.append(RegularField(group[0]))
        else:
            dominant = BITS_TO_UNSIGNED.get(group_unit, f"uint{group_unit}_t")
            elements.append(BitfieldGroup(
                fields=list(group),
                unit_bits=group_unit,
                dominant_type=dominant,
            ))
        group = []
        group_bits = 0
        group_unit = 0

    for fld in fields:
        # --- non-bitfield: close any open group, emit as regular ---
        if fld.bit_width is None:
            _close_group()
            elements.append(RegularField(fld))
            continue

        # --- zero-width bitfield (alignment boundary) ---
        if fld.bit_width == 0:
            _close_group()
            continue

        # --- unnamed padding bitfield ---
        if not fld.name:
            _close_group()
            elements.append(RegularField(fld))
            continue

        # --- normal named bitfield ---
        fld_type_bits = TYPE_BITS.get(fld.type_name, 0)
        if fld_type_bits == 0:
            # Unknown bitfield base type — treat as regular field
            _close_group()
            elements.append(RegularField(fld))
            continue

        # Would this field fit in the current (possibly expanded) unit?
        new_unit = max(group_unit, fld_type_bits)
        if group_bits + fld.bit_width <= new_unit:
            group.append(fld)
            group_bits += fld.bit_width
            group_unit = new_unit
            # If unit is full, close the group
            if group_bits >= group_unit:
                _close_group()
        else:
            # Overflow — close current group, start a new one
            _close_group()
            group = [fld]
            group_bits = fld.bit_width
            group_unit = fld_type_bits
            if group_bits >= group_unit:
                _close_group()

    _close_group()
    return elements
