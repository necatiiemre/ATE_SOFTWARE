"""
Data models for representing parsed C header structures.

This module defines the core data structures used throughout the
endian converter pipeline: parsing, traversal, and code generation.
"""

from dataclasses import dataclass, field
from typing import Optional, Dict, List, Tuple


@dataclass
class FieldDef:
    """Represents a single field within a struct definition.

    Attributes:
        name:            Field identifier (e.g. "messageId").
        type_name:       Original type spelling (e.g. "uint32_t", "StatusCode").
        type_category:   One of "primitive", "enum", "struct", "unknown".
        is_array:        True when the field is a fixed-size array.
        array_size:      Evaluated numeric element count (may be None).
        array_size_expr: Original source expression (e.g. "MAX_ITEMS").
        bit_width:       Bit-field width, or None for non-bitfield fields.
    """
    name: str
    type_name: str
    type_category: str
    is_array: bool = False
    array_size: Optional[int] = None
    array_size_expr: Optional[str] = None
    bit_width: Optional[int] = None


@dataclass
class EnumDef:
    """Represents an enum type definition.

    Attributes:
        name:   Typedef or tag name of the enum.
        values: Ordered list of (name, integer_value) pairs.
    """
    name: str
    values: List[Tuple[str, int]] = field(default_factory=list)


@dataclass
class StructDef:
    """Represents a struct type definition.

    Attributes:
        name:      Typedef or tag name of the struct.
        fields:    Ordered list of field definitions.
        is_packed: True when __attribute__((packed)) is present.
    """
    name: str
    fields: List[FieldDef] = field(default_factory=list)
    is_packed: bool = False


@dataclass
class HeaderModel:
    """Complete parsed representation of a C header file.

    Attributes:
        filename:         Base name of the source header file.
        structs:          Mapping of struct name -> StructDef.
        enums:            Mapping of enum name  -> EnumDef.
        root_struct_name: Name of the top-level root struct (set by CLI).
        includes:         Original #include directives from the source.
        defines:          Original #define directives (excluding header guard).
    """
    filename: str
    structs: Dict[str, StructDef] = field(default_factory=dict)
    enums: Dict[str, EnumDef] = field(default_factory=dict)
    root_struct_name: Optional[str] = None
    includes: List[str] = field(default_factory=list)
    defines: List[str] = field(default_factory=list)

    def summary(self) -> str:
        """Return a human-readable summary of the parsed model."""
        lines = [f"Header: {self.filename}"]

        lines.append(f"  Enums ({len(self.enums)}):")
        for name, enum_def in self.enums.items():
            names = [n for n, _ in enum_def.values]
            lines.append(f"    {name}: {', '.join(names)}")

        lines.append(f"  Structs ({len(self.structs)}):")
        for name, struct_def in self.structs.items():
            packed_tag = " [packed]" if struct_def.is_packed else ""
            lines.append(f"    {name}{packed_tag}:")
            for f in struct_def.fields:
                arr = f"[{f.array_size_expr or f.array_size}]" if f.is_array else ""
                bf = f" :{f.bit_width}" if f.bit_width is not None else ""
                lines.append(
                    f"      {f.type_name}{arr} {f.name}{bf} ({f.type_category})"
                )

        return "\n".join(lines)
