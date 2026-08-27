C Header Endian Converter
=========================

Reads a C header file that defines structs and enums, 
and generates a new header file containing:

  - Endianness detection macros
  - Reproduced enum and struct definitions (bitfield groups reordered for little-endian / big-endian targets)
  - static inline endian conversion function
  - static inline CSV header-print function
  - static inline CSV data-print function

No runtime dependencies beyond the bundled libs/ directory.

Usage
-----
    python run.py <input.h> --root <RootStructName> [-o output.h] [-v]

Arguments:
  input             Path to the C header file to parse.
  --root NAME       Name of the top-level struct (required).
  -o / --output     Output file path.
                    Default: <input_stem>_PARSED.h next to the input file.
  -v / --verbose    Print the parsed model summary before generating.

Examples:
    python run.py tests/LOG_TEST.h --root TelemetryLog
    python run.py my_log.h --root MyStruct -o out/my_log_PARSED.h -v


What the input header contains
------------------------------------
The header is a standard C header.  

The tool understands:

  Includes (preserved as-is):
      #include <stdint.h>
      #include <stdbool.h>

  Defines (preserved as-is):
      #define MAX_CHANNELS  4
      #define MAX_SAMPLES  16

  Enums (typedef style):
      typedef enum {
          CHANNEL_TYPE_ANALOG  = 0,
          CHANNEL_TYPE_DIGITAL = 1,
      } ChannelType;

  Structs (typedef style, optionally packed):
      typedef struct __attribute__((packed)) {
          uint32_t fileId;
          uint64_t creationTimestamp;
          uint8_t  compressionLevel : 4;   // bitfield
          uint8_t  encryptionType   : 2;   // bitfield
          uint8_t  reserved1        : 2;   // skipped in output functions
      } FileHeader;

  Nested structs and fixed-size arrays (macro or literal size):
      Sample   samples[MAX_SAMPLES];
      Marker   markers[8];

Supported field types inside structs:
  uint8_t, uint16_t, uint32_t, uint64_t
  int8_t,  int16_t,  int32_t,  int64_t
  bool
  Enums defined in the same header
  Nested structs defined in the same header
  Bitfields of the above integer types

Fields named "reserved*" are silently skipped in all generated functions.


What the output header contains
---------------------------------
1. Endianness detection block:

    #if defined(__BYTE_ORDER__) && ...
        #define SYSTEM_LITTLE_ENDIAN 1
    #elif ...
        #define SYSTEM_BIG_ENDIAN 1
    #else
        #error "Unknown endianness!"
    #endif

2. #include directives from the input (plus <stdio.h>).

3. #define macros from the input.

4. Enum definitions (reformatted, same values).

5. Struct definitions with bitfield groups wrapped in conditional blocks:

    #if defined(SYSTEM_LITTLE_ENDIAN)
        uint8_t reserved1        : 2;   // reversed order
        uint8_t encryptionType   : 2;
        uint8_t compressionLevel : 4;
    #elif defined(SYSTEM_BIG_ENDIAN)
        uint8_t compressionLevel : 4;   // original order
        uint8_t encryptionType   : 2;
        uint8_t reserved1        : 2;
    #endif

Bitfield Type Normalization
When grouping consecutive bitfields for endianness reordering,
the script normalizes mixed data types to the widest type in 
the group to prevent compiler padding issues.

// Input: Mixed types (8-bit and 16-bit)
uint8_t  flagA : 4;
uint16_t flagB : 6;

// Output: Normalized to uint16_t and reordered
#if defined(SYSTEM_LITTLE_ENDIAN)
    uint16_t flagB : 6;
    uint16_t flagA : 4;
#elif defined(SYSTEM_BIG_ENDIAN)
    ...

6. Endian conversion function (uses ntohs / ntohl / ntohll):

    static inline void log_test_endian_convert(TelemetryLog* data) {
        // recurses into nested structs and arrays automatically
        data->header.fileId = ntohl(data->header.fileId);
        ...
    }

7. CSV header-print function (prints field path names):

    static inline void log_test_header_print(FILE *fp) {
        // one comma-separated column name per leaf field
        fprintf(fp, "header.fileId,");
        ...
        fprintf(fp, "isFinalized\n");
    }

8. CSV data-print function (prints field values):

    static inline void log_test_data_print(FILE *fp, TelemetryLog* data) {
        fprintf(fp, "%u,", data->header.fileId);
        ...
        fprintf(fp, "%u\n", (unsigned int)data->isFinalized);
    }

Current capabilities
--------------------
  - Parses arbitrarily nested struct hierarchies.
  - Handles fixed-size arrays at any nesting level (generates for-loops).
  - Detects and reorders bitfields for little-endian vs big-endian.
  - Skips bitfield fields in the endian-convert function (bitfields cannot be passed to ntohs/ntohl directly).
  - Skips fields named "reserved*" everywhere.
  - Preserves original array-size macro expressions (MAX_SAMPLES, etc.).
  - Detects __attribute__((packed)) and reproduces it in output.
  - Carries original #include and #define lines into the output.


