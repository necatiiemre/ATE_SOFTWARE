"""
Generates a test_main.c that reads a binary .log file using the
functions from a PARSED.h header and writes CSV output.

The generated C program:
  1. Skips prefix structs (fread to advance cursor)
  2. Calls header_print once (CSV column names)
  3. Loops over root struct: fread + data_print (CSV data rows)
"""

from typing import List, Tuple


def generate_test_main(
    parsed_header_name: str,
    log_filename: str,
    csv_filename: str,
    struct_sequence: List[Tuple[str, int]],
    file_prefix: str,
) -> str:
    """Generate C source for test_main.c."""
    root_name = struct_sequence[-1][0]
    root_count = struct_sequence[-1][1]
    prefix_structs = struct_sequence[:-1]

    lines: List[str] = []

    # Portable byte-swap utilities
    lines.append("#include <stdio.h>")
    lines.append("#include <stdlib.h>")
    lines.append("#include <stdint.h>")
    lines.append("#include <stdbool.h>")
    lines.append("")
    lines.append("/* Portable byte-swap */")
    lines.append("static inline uint16_t _bswap16(uint16_t x) {")
    lines.append("    return (uint16_t)((x >> 8) | (x << 8));")
    lines.append("}")
    lines.append("static inline uint32_t _bswap32(uint32_t x) {")
    lines.append("    return ((x >> 24) & 0xFF)     | ((x >> 8) & 0xFF00) |")
    lines.append("           ((x << 8)  & 0xFF0000) | ((x << 24));")
    lines.append("}")
    lines.append("static inline uint64_t _bswap64(uint64_t x) {")
    lines.append("    return ((uint64_t)_bswap32((uint32_t)(x & 0xFFFFFFFF)) << 32) |")
    lines.append("           _bswap32((uint32_t)(x >> 32));")
    lines.append("}")
    lines.append("#define ntohs _bswap16")
    lines.append("#define ntohl _bswap32")
    lines.append("#define ntohll _bswap64")
    lines.append("")
    lines.append(f'#include "{parsed_header_name}"')
    lines.append("")

    # main
    lines.append("int main(void) {")
    lines.append(f'    FILE* log_fp = fopen("{log_filename}", "rb");')
    lines.append("    if (!log_fp) {")
    lines.append('        printf("ERROR: Cannot open log file\\n");')
    lines.append("        return 1;")
    lines.append("    }")
    lines.append(f'    FILE* csv_fp = fopen("{csv_filename}", "w");')
    lines.append("    if (!csv_fp) {")
    lines.append('        printf("ERROR: Cannot open CSV output file\\n");')
    lines.append("        return 1;")
    lines.append("    }")
    lines.append("")

    # Skip prefix structs
    for struct_name, count in prefix_structs:
        lines.append(f"    /* Skip {struct_name} ({count}x) */")
        lines.append("    {")
        lines.append(f"        {struct_name} skip;")
        for _ in range(count):
            lines.append(
                f"        fread(&skip, sizeof({struct_name}), 1, log_fp);"
            )
        lines.append(
            f'        printf("Skipped {struct_name} (%zu bytes)\\n", '
            f"sizeof({struct_name}));"
        )
        lines.append("    }")
        lines.append("")

    # Read + convert + print root structs
    lines.append(f"    /* Process {root_name} ({root_count}x) */")
    lines.append(f"    {file_prefix}_header_print(csv_fp);")
    lines.append(f"    for (int n = 0; n < {root_count}; n++) {{")
    lines.append(f"        {root_name} data;")
    lines.append(
        f"        size_t rd = fread(&data, sizeof({root_name}), 1, log_fp);"
    )
    lines.append("        if (rd != 1) {")
    lines.append(
        f'            printf("ERROR: fread {root_name} failed at index %d\\n", n);'
    )
    lines.append("            break;")
    lines.append("        }")
    lines.append(f"        {file_prefix}_data_print(csv_fp, &data);")
    lines.append("    }")
    lines.append("")
    lines.append("    fclose(log_fp);")
    lines.append("    fclose(csv_fp);")
    lines.append(f'    printf("CSV written to {csv_filename}\\n");')
    lines.append("    return 0;")
    lines.append("}")
    lines.append("")

    return "\n".join(lines)
