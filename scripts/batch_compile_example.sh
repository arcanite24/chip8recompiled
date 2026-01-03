#!/bin/bash
# Example script to batch compile CHIP-8 ROMs
# Usage: ./batch_compile_example.sh

set -e  # Exit on error

echo "=== CHIP-8 Multi-ROM Batch Compilation Example ==="
echo

# Configuration
ROMS_DIR="${CHIP8_ROMS_DIR:-chip8Archive/roms}"
OUTPUT_DIR="${OUTPUT_DIR:-build/multi_rom_launcher}"
CHIP8RECOMP="${CHIP8RECOMP:-build/recompiler/chip8recomp}"

# Check if recompiler exists
if [ ! -f "$CHIP8RECOMP" ]; then
    echo "Error: chip8recomp not found at $CHIP8RECOMP"
    echo "Please build the recompiler first:"
    echo "  mkdir build && cd build"
    echo "  cmake -G Ninja .."
    echo "  cmake --build ."
    exit 1
fi

# Check if ROMs directory exists
if [ ! -d "$ROMS_DIR" ]; then
    echo "Error: ROMs directory not found at $ROMS_DIR"
    echo "Please set CHIP8_ROMS_DIR environment variable or ensure chip8Archive is cloned"
    exit 1
fi

# Count ROMs
ROM_COUNT=$(find "$ROMS_DIR" -name "*.ch8" -o -name "*.chip8" | wc -l)
echo "Found $ROM_COUNT ROM(s) in $ROMS_DIR"
echo

# Run batch compilation
echo "Running batch compilation..."
echo "Command: $CHIP8RECOMP --batch \"$ROMS_DIR\" -o \"$OUTPUT_DIR\""
echo

"$CHIP8RECOMP" --batch "$ROMS_DIR" -o "$OUTPUT_DIR"

# Check if compilation succeeded
if [ $? -ne 0 ]; then
    echo
    echo "Error: Batch compilation failed"
    exit 1
fi

echo
echo "=== Compilation Complete ==="
echo
echo "Generated files are in: $OUTPUT_DIR"
echo
echo "To build and run the multi-ROM launcher:"
echo "  cd $OUTPUT_DIR"
echo "  mkdir build && cd build"
echo "  cmake -G Ninja .."
echo "  cmake --build ."
echo "  ./chip8_launcher"
echo
