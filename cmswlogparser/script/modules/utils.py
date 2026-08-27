"""
Utility functions for the endian converter.
"""

import os


def get_file_prefix(filepath: str) -> str:
    """Extract the filename stem (without extension) from a path.

    Examples:
        >>> get_file_prefix("input_log_file.h")
        'input_log_file'
        >>> get_file_prefix("/some/path/my_header.h")
        'my_header'
    """
    basename = os.path.basename(filepath)
    stem, _ = os.path.splitext(basename)
    return stem.lower()


def get_index_variable(depth: int) -> str:
    """Return a loop index variable name for the given nesting depth.

    Produces i, j, k, l, ... for depths 0, 1, 2, 3, ...
    Falls back to idx0, idx1, ... for depths beyond 26.
    """
    variables = "ijklmnopqrstuvwxyz"
    if depth < len(variables):
        return variables[depth]
    return f"idx{depth}"
