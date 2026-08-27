"""
C header file parser using libclang.

Parses struct and enum definitions from C header files,
building a HeaderModel that represents the type hierarchy.
Handles typedefs, nested structs, arrays with macro sizes,
and __attribute__((packed)).
"""

import os
import re
from typing import List, Optional, Tuple

import clang.cindex
from clang.cindex import CursorKind, TypeKind

from .model import HeaderModel, StructDef, EnumDef, FieldDef


# ---------------------------------------------------------------------------
# Recognized leaf primitive types
# ---------------------------------------------------------------------------
PRIMITIVE_TYPES = frozenset({
    "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "int8_t",  "int16_t",  "int32_t",  "int64_t",
    "bool", "_Bool",
})


class HeaderParser:
    """Parses a single C header file into a *HeaderModel*."""

    def __init__(
        self,
        filepath: str,
        extra_args: Optional[List[str]] = None,
    ):
        self.filepath = os.path.abspath(filepath)
        self.extra_args = extra_args or []
        self._source_lines: List[str] = []
        self._model = HeaderModel(filename=os.path.basename(filepath))

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def parse(self) -> HeaderModel:
        """Parse the header file and return a *HeaderModel*."""
        # Keep the raw source for array-size-expression extraction
        with open(self.filepath, "r", encoding="utf-8", errors="replace") as fh:
            self._source_lines = fh.readlines()

        index = clang.cindex.Index.create()

        # Compiler arguments
        args = ["-x", "c", "-std=c11",
               "-include", "stdint.h",
               "-include", "stdbool.h"]

        # Fallback include directory shipped with the project
        include_dir = os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            "include",
        )
        if os.path.isdir(include_dir):
            args.extend(["-isystem", include_dir])

        args.extend(self.extra_args)

        tu = index.parse(self.filepath, args=args)
        if tu is None:
            raise RuntimeError(f"libclang failed to parse: {self.filepath}")

        # Report diagnostics
        has_errors = False
        for diag in tu.diagnostics:
            if diag.severity >= clang.cindex.Diagnostic.Error:
                print(f"  PARSE ERROR: {diag.spelling}  ({diag.location})")
                has_errors = True
            elif diag.severity == clang.cindex.Diagnostic.Warning:
                print(f"  PARSE WARNING: {diag.spelling}")

        if has_errors:
            raise RuntimeError(
                f"Unrecoverable parse errors in: {self.filepath}"
            )

        self._walk_top_level(tu.cursor)
        self._extract_preprocessor_directives()
        return self._model

    # ------------------------------------------------------------------
    # AST walking
    # ------------------------------------------------------------------

    def _walk_top_level(self, root_cursor) -> None:
        """Iterate over top-level declarations in the target file."""
        for cursor in root_cursor.get_children():
            if not self._is_from_target_file(cursor):
                continue

            if cursor.kind == CursorKind.TYPEDEF_DECL:
                self._process_typedef(cursor)
            elif cursor.kind == CursorKind.STRUCT_DECL and cursor.spelling:
                self._process_struct_decl(cursor, cursor.spelling)
            elif cursor.kind == CursorKind.ENUM_DECL and cursor.spelling:
                self._process_enum_decl(cursor, cursor.spelling)

    # ------------------------------------------------------------------
    # Typedef handling
    # ------------------------------------------------------------------

    def _process_typedef(self, cursor) -> None:
        """Resolve a typedef to its underlying struct / enum definition."""
        typedef_name = cursor.spelling
        underlying = cursor.underlying_typedef_type
        canonical = underlying.get_canonical()

        if canonical.kind == TypeKind.RECORD:
            for child in cursor.get_children():
                if child.kind == CursorKind.STRUCT_DECL:
                    self._process_struct_decl(child, typedef_name)
                    return
        elif canonical.kind == TypeKind.ENUM:
            for child in cursor.get_children():
                if child.kind == CursorKind.ENUM_DECL:
                    self._process_enum_decl(child, typedef_name)
                    return

    # ------------------------------------------------------------------
    # Struct / Enum processing
    # ------------------------------------------------------------------

    def _process_struct_decl(self, cursor, name: str) -> None:
        struct_def = StructDef(name=name)
        struct_def.is_packed = self._detect_packed(cursor)

        for child in cursor.get_children():
            if child.kind == CursorKind.FIELD_DECL:
                field_def = self._process_field(child)
                if field_def is not None:
                    struct_def.fields.append(field_def)

        self._model.structs[name] = struct_def

    def _process_enum_decl(self, cursor, name: str) -> None:
        enum_def = EnumDef(name=name)

        for child in cursor.get_children():
            if child.kind == CursorKind.ENUM_CONSTANT_DECL:
                enum_def.values.append((child.spelling, child.enum_value))

        self._model.enums[name] = enum_def

    # ------------------------------------------------------------------
    # Field processing
    # ------------------------------------------------------------------

    def _process_field(self, cursor) -> Optional[FieldDef]:
        """Build a *FieldDef* from a FIELD_DECL cursor."""
        field_name = cursor.spelling
        field_type = cursor.type

        is_array = False
        array_size: Optional[int] = None
        array_size_expr: Optional[str] = None

        # Unwrap array types
        if field_type.kind == TypeKind.CONSTANTARRAY:
            is_array = True
            array_size = field_type.element_count
            array_size_expr = self._extract_array_size_expr(cursor)
            if array_size_expr is None:
                array_size_expr = str(array_size)
            element_type = field_type.element_type
        elif field_type.kind == TypeKind.INCOMPLETEARRAY:
            is_array = True
            array_size_expr = self._extract_array_size_expr(cursor)
            element_type = field_type.element_type
        else:
            element_type = field_type

        type_name, type_category = self._resolve_type(element_type)

        # Detect bitfield
        bit_width = None
        if cursor.is_bitfield():
            bit_width = cursor.get_bitfield_width()

        return FieldDef(
            name=field_name,
            type_name=type_name,
            type_category=type_category,
            is_array=is_array,
            array_size=array_size,
            array_size_expr=array_size_expr,
            bit_width=bit_width,
        )

    # ------------------------------------------------------------------
    # Type resolution
    # ------------------------------------------------------------------

    def _resolve_type(self, clang_type) -> Tuple[str, str]:
        """Map a libclang Type to (type_name, type_category)."""
        spelling = clang_type.spelling.replace("const ", "").replace(
            "volatile ", ""
        ).strip()

        # Direct primitive match
        if spelling in PRIMITIVE_TYPES:
            return spelling, "primitive"

        canonical = clang_type.get_canonical()

        # Boolean
        if canonical.kind == TypeKind.BOOL:
            return "bool", "primitive"

        # Enum
        if canonical.kind == TypeKind.ENUM:
            name = spelling
            if name.startswith("enum "):
                name = name[5:]
            return name, "enum"

        # Struct / Record
        if canonical.kind == TypeKind.RECORD:
            name = spelling
            if name.startswith("struct "):
                name = name[7:]
            return name, "struct"

        # Integer-like typedef we do not explicitly recognize
        _INTEGER_KINDS = {
            TypeKind.UCHAR, TypeKind.USHORT, TypeKind.UINT,
            TypeKind.ULONG, TypeKind.ULONGLONG,
            TypeKind.SCHAR, TypeKind.SHORT, TypeKind.INT,
            TypeKind.LONG, TypeKind.LONGLONG,
            TypeKind.CHAR_S, TypeKind.CHAR_U,
        }
        if canonical.kind in _INTEGER_KINDS:
            if spelling in PRIMITIVE_TYPES:
                return spelling, "primitive"
            print(
                f"  WARNING: Unrecognized integer typedef '{spelling}' "
                f"(canonical: {canonical.spelling})"
            )
            return spelling, "unknown"

        # Truly unknown
        print(
            f"  WARNING: Unknown type '{spelling}' "
            f"(kind: {canonical.kind})"
        )
        return spelling, "unknown"

    # ------------------------------------------------------------------
    # Source-text helpers
    # ------------------------------------------------------------------

    def _extract_array_size_expr(self, field_cursor) -> Optional[str]:
        """Read the original array-size token from the source text.

        For ``DataItem items[MAX_ITEMS];`` this returns ``"MAX_ITEMS"``.
        """
        try:
            extent = field_cursor.extent
            start_line = extent.start.line - 1
            end_line = extent.end.line - 1

            if start_line == end_line:
                text = self._source_lines[start_line]
            else:
                text = "".join(
                    self._source_lines[start_line : end_line + 1]
                )

            match = re.search(r"\[(.+?)\]", text)
            if match:
                return match.group(1).strip()
        except (IndexError, AttributeError):
            pass
        return None

    def _detect_packed(self, cursor) -> bool:
        """Heuristic check for ``__attribute__((packed))``.
        
        Scans the entire struct extent so it catches both styles:
          typedef struct __attribute__((packed)) { ... } Name;
          typedef struct { ... } __attribute__((packed)) Name;
        """
        try:
            start_line = cursor.extent.start.line - 1
            end_line = cursor.extent.end.line #inclusive
            # Also check one line before (typedef line) and one after
            scan_start = max(start_line - 1, 0)
            scan_end = min(end_line + 1, len(self._source_lines))
            for idx in range(scan_start, scan_end):
                if "packed" in self._source_lines[idx]:
                    return True
        except (IndexError, AttributeError):
            pass
        return False

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _is_from_target_file(self, cursor) -> bool:
        if cursor.location.file is None:
            return False
        return os.path.abspath(cursor.location.file.name) == self.filepath

    def _extract_preprocessor_directives(self) -> None:
        """Scan source text for ``#include`` and ``#define`` directives.

        Results are stored in ``self._model.includes`` and
        ``self._model.defines``.  The header guard ``#define`` is
        excluded automatically.
        """
        header_guard_name = None

        for line in self._source_lines:
            stripped = line.strip()

            if stripped.startswith("#include"):
                self._model.includes.append(stripped)

            elif stripped.startswith("#ifndef") and header_guard_name is None:
                parts = stripped.split()
                if len(parts) >= 2:
                    header_guard_name = parts[1]

            elif stripped.startswith("#define"):
                parts = stripped.split(None, 2)
                if len(parts) >= 2 and parts[1] != header_guard_name:
                    self._model.defines.append(stripped)
