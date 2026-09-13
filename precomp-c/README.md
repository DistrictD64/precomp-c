# Precomp C - C Implementation for Tiny C Compiler

This is a C conversion of the precomp-cpp project, designed to be compilable with the Tiny C Compiler (TCC).

## Overview

The original precomp-cpp is a C++17 project (~8,300 lines) that uses many C++ features incompatible with TCC:
- Classes and inheritance
- STL containers (vector, map, string, etc.)
- Templates
- Smart pointers
- Exceptions
- Lambda functions
- Threads and mutexes

This C version replaces these with:
- Structs with function pointer vtables
- Manual memory management
- Error codes instead of exceptions
- Simple callback functions
- Platform-specific implementations

## Files Created

### Core Library
- `precomp_lib.h` - Main public API header (C-friendly)
- `precomp_lib.c` - Library implementation
- `precomp.h` - Internal core structures
- `precomp.c` - Core Precomp class implementation
- `precomp_io.h` - I/O stream interface
- `precomp_io.c` - I/O implementation
- `precomp_utils.h` - Utility functions header
- `precomp_utils.c` - Utility functions implementation

### Test Program
- `test_precomp.c` - Example usage program

## Building with TCC

```bash
# Compile the library
tcc -c precomp_utils.c -o precomp_utils.o
tcc -c precomp_io.c -o precomp_io.o
tcc -c precomp.c -o precomp.o
tcc -c precomp_lib.c -o precomp_lib.o

# Create static library
tcc -ar libprecomp.a precomp_utils.o precomp_io.o precomp.o precomp_lib.o

# Compile test program
tcc test_precomp.c -L. -lprecomp -o precomp_test.exe
```

Or compile all at once:
```bash
tcc precomp_utils.c precomp_io.c precomp.c precomp_lib.c test_precomp.c -o precomp_test.exe
```

## Usage

```bash
# Decompress a .pcf file
./precomp_test d input.pcf output.bin

# Compress/precompress a file
./precomp_test c input.bin output.pcf
```

## API Overview

### Basic Usage
```c
#include "precomp_lib.h"

// Create instance
PcompInstance* inst = pcomp_create();

// Configure
PcompSwitches* sw = pcomp_get_switches(inst);
pcomp_switch_set_jpg(sw, true);
pcomp_switch_set_png(sw, true);

// Set callbacks
pcomp_set_progress_callback(inst, my_progress_fn);
pcomp_set_log_callback(my_log_fn);

// Open files
FILE* in = fopen("input.bin", "rb");
FILE* out = fopen("output.pcf", "wb");

pcomp_set_input_file(inst, in, "input.bin");
pcomp_set_output_file(inst, out, "output.pcf");

// Precompress
int result = pcomp_precompress(inst);

// Cleanup
fclose(in);
fclose(out);
pcomp_destroy(inst);
```

## Current Status

### Implemented
- [x] Core data structures
- [x] I/O stream abstraction
- [x] Utility functions
- [x] Main Precomp class skeleton
- [x] C-friendly API wrapper
- [x] Test program

### TODO / Incomplete
- [ ] Full precompression logic (format detection, compression)
- [ ] Full recompression logic (header parsing, decompression)
- [ ] Format handlers (PDF, ZIP, PNG, JPEG, GIF, MP3, BZIP2)
- [ ] Deflate/zlib integration
- [ ] Recursion support
- [ ] Intense/brute modes
- [ ] Threading support (if needed for TCC)
- [ ] Complete error handling

## Notes

1. **Memory Management**: All allocations must be freed manually. The library provides destroy functions for each type.

2. **Error Handling**: Uses return codes instead of exceptions. Check return values!

3. **Thread Safety**: Currently not thread-safe. TCC doesn't have built-in threading like C++17.

4. **Platform Support**: Works on Windows and Unix-like systems. Binary mode is automatic on Unix.

5. **Dependencies**: This pure C version removes dependencies on Boost, C++ STL, and other C++ libraries. External compression libraries (zlib, bzip2, etc.) would need C wrappers or direct C implementations.

## Comparison with Original

| Feature | C++ Version | C Version |
|---------|-------------|-----------|
| Lines of Code | ~8,300 | ~1,500 (core so far) |
| Compiler | GCC/Clang (C++17) | TCC/GCC (C99) |
| Memory | RAII, smart pointers | Manual malloc/free |
| Errors | Exceptions | Return codes |
| Polymorphism | Virtual methods | Function pointers |
| Containers | std::vector, std::map | Arrays, manual lists |
| Strings | std::string | char* |

## Next Steps

To complete the conversion, each format handler and compression algorithm needs to be converted from C++ to C following the same patterns used in the core files.
