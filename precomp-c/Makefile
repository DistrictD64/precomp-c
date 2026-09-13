# Makefile for precomp-c
# Cross-platform build system for Tiny C Compiler (TCC)
# Works on Linux, macOS, BSD, and Windows (with GNU Make)

# Directories
BUILD_DIR := build
SRC_DIR := src
INCLUDE_DIR := include
CONTRIB_DIR := contrib

# Compiler settings
CC ?= tcc
AR ?= ar

# Detect OS
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
ifeq ($(UNAME_S),Linux)
    OS_FLAGS := -D_GNU_SOURCE
    EXE_EXT :=
else ifeq ($(UNAME_S),Darwin)
    OS_FLAGS := -D_DARWIN_C_SOURCE
    EXE_EXT :=
else ifeq ($(findstring BSD,$(UNAME_S)),BSD)
    OS_FLAGS := -D_BSD_SOURCE
    EXE_EXT :=
else
    OS_FLAGS := -D_WIN32 -D_CRT_SECURE_NO_WARNINGS
    EXE_EXT := .exe
endif

# Compiler flags
CFLAGS := -I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/formats
CFLAGS += -I$(CONTRIB_DIR)/zlib -I$(CONTRIB_DIR)/bzip2
CFLAGS += -I$(CONTRIB_DIR)/giflib -I$(CONTRIB_DIR)/libjpeg -I$(CONTRIB_DIR)/libpng
CFLAGS += $(OS_FLAGS)

# Build type
ifdef DEBUG
    CFLAGS += -g -DDEBUG
    BUILD_TYPE := debug
else
    CFLAGS += -O2
    BUILD_TYPE := release
endif

# Target
TARGET := $(BUILD_DIR)/precomp$(EXE_EXT)

# Source files
CORE_SRCS := $(wildcard $(SRC_DIR)/*.c)
FORMAT_SRCS := $(wildcard $(SRC_DIR)/formats/*.c)
ZLIB_SRCS := $(wildcard $(CONTRIB_DIR)/zlib/*.c)
BZIP2_SRCS := $(wildcard $(CONTRIB_DIR)/bzip2/*.c)
GIFLIB_SRCS := $(wildcard $(CONTRIB_DIR)/giflib/*.c)
LIBJPEG_SRCS := $(wildcard $(CONTRIB_DIR)/libjpeg/*.c)
LIBPNG_SRCS := $(wildcard $(CONTRIB_DIR)/libpng/*.c)

ALL_SRCS := $(CORE_SRCS) $(FORMAT_SRCS) $(ZLIB_SRCS) $(BZIP2_SRCS) \
            $(GIFLIB_SRCS) $(LIBJPEG_SRCS) $(LIBPNG_SRCS)

# Object files
OBJ_DIR := $(BUILD_DIR)/obj
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(ALL_SRCS)))

# Default target
.PHONY: all
all: $(TARGET)
	@echo "[INFO] Build complete: $(TARGET)"
	@ls -lh $(TARGET) 2>/dev/null || dir $(TARGET) 2>/dev/null || true

# Create directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Link target
$(TARGET): $(OBJS) | $(BUILD_DIR)
	@echo "[INFO] Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $(OBJS)
	@echo "[INFO] Build successful!"

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/formats/%.c | $(OBJ_DIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(CONTRIB_DIR)/zlib/%.c | $(OBJ_DIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(CONTRIB_DIR)/bzip2/%.c | $(OBJ_DIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(CONTRIB_DIR)/giflib/%.c | $(OBJ_DIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(CONTRIB_DIR)/libjpeg/%.c | $(OBJ_DIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(CONTRIB_DIR)/libpng/%.c | $(OBJ_DIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c -o $@ $<

# Debug build
.PHONY: debug
debug:
	$(MAKE) DEBUG=1

# Release build
.PHONY: release
release:
	$(MAKE) DEBUG=

# Clean
.PHONY: clean
clean:
	@echo "[INFO] Cleaning..."
	rm -rf $(BUILD_DIR)
	@echo "[INFO] Clean complete"

# Install
.PHONY: install
install: $(TARGET)
	@echo "[INFO] Installing to $(PREFIX)..."
	@mkdir -p $(PREFIX)/bin
	@mkdir -p $(PREFIX)/share/precomp
	cp $(TARGET) $(PREFIX)/bin/
	@echo "[INFO] Installation complete"

# Uninstall
.PHONY: uninstall
uninstall:
	@echo "[INFO] Uninstalling..."
	rm -f $(PREFIX)/bin/precomp$(EXE_EXT)
	rm -rf $(PREFIX)/share/precomp
	@echo "[INFO] Uninstall complete"

# Help
.PHONY: help
help:
	@echo "precomp-c Makefile"
	@echo "=================="
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build precomp (default)"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build with optimizations"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to PREFIX (default: /usr/local)"
	@echo "  uninstall - Remove installed files"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  CC        - C compiler (default: tcc)"
	@echo "  DEBUG     - Set to 1 for debug build"
	@echo "  PREFIX    - Installation prefix (default: /usr/local)"
	@echo ""
	@echo "Examples:"
	@echo "  make                  # Build release version"
	@echo "  make debug            # Build debug version"
	@echo "  make clean            # Clean build"
	@echo "  make install PREFIX=/opt/precomp"
	@echo "  CC=gcc make           # Use GCC instead of TCC"

# Show configuration
.PHONY: config
config:
	@echo "Configuration:"
	@echo "  OS: $(UNAME_S)"
	@echo "  Compiler: $(CC)"
	@echo "  Build type: $(BUILD_TYPE)"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo "  Target: $(TARGET)"
	@echo "  Sources: $(words $(ALL_SRCS)) files"
