# Multi-ROM Launcher Feature

This feature allows you to compile multiple CHIP-8 ROMs into a single executable with an integrated ROM selection menu.

## Quick Start

### 1. Batch Compile ROMs

```bash
# Compile all ROMs in a directory
chip8recomp --batch path/to/roms/ -o multi_rom_output
```

### 2. Build the Launcher

```bash
cd multi_rom_output
mkdir build && cd build
cmake -G Ninja ..
cmake --build .
```

### 3. Run

```bash
./chip8_launcher
```

You'll see a menu where you can select which ROM to play. During gameplay, press ESC and select "Back to Menu" to return to the ROM selection screen.

## Features

- **Visual ROM Selection**: ImGui-based menu with mouse and keyboard support
- **ROM Switching**: Switch between ROMs without restarting the application
- **Metadata Display**: Shows ROM title, author, description (when available)
- **Pause Menu Integration**: "Back to Menu" option in the pause menu
- **Settings Support**: Each ROM can have its own settings
- **Fast Compilation**: Batch processes all ROMs in one command

## Usage Examples

### Example 1: Compile Sample ROMs

```bash
# Compile all ROMs from chip8Archive
chip8recomp --batch chip8Archive/roms -o game_collection

# Build
cd game_collection
mkdir build && cd build
cmake -G Ninja ..
cmake --build .

# Run
./chip8_launcher
```

### Example 2: With Options

```bash
# Compile with generator options
chip8recomp --batch roms/ \
    -o output/ \
    --no-comments \
    --single-function

# This applies the options to all ROMs in the batch
```

### Example 3: Using the Helper Script

```bash
# Set environment variables
export CHIP8_ROMS_DIR=/path/to/roms
export OUTPUT_DIR=my_launcher

# Run the batch compile script
./scripts/batch_compile_example.sh
```

## How It Works

### Compilation Phase

1. **Directory Scan**: Finds all `.ch8` and `.chip8` files
2. **Individual Compilation**: Each ROM is compiled separately with a unique namespace prefix
3. **Catalog Generation**: Creates a C array of `RomEntry` structures
4. **Launcher Generation**: Generates `main.c` that calls `chip8_run_with_menu()`
5. **Build Config**: Creates unified `CMakeLists.txt` for all ROMs

### Runtime Phase

1. **Menu Display**: Shows list of available ROMs with metadata
2. **ROM Selection**: User chooses a ROM to play
3. **Context Setup**: Clears function table, registers ROM's functions
4. **Execution**: Runs the selected ROM
5. **Menu Return**: User can return to menu from pause menu

## Generated File Structure

```
output/
├── pong.c                    # Compiled ROM (Pong)
├── pong.h
├── pong_rom_data.c           # Embedded ROM data
├── tetris.c                  # Compiled ROM (Tetris)
├── tetris.h
├── tetris_rom_data.c
├── breakout.c                # Compiled ROM (Breakout)
├── breakout.h
├── breakout_rom_data.c
├── rom_catalog.c             # Catalog of all ROMs
├── main.c                    # Multi-ROM launcher entry
└── CMakeLists.txt            # Build configuration
```

## Keyboard Controls

### ROM Selection Menu
- **↑/↓**: Navigate ROM list
- **Enter**: Launch selected ROM
- **Mouse Click**: Select and launch ROM
- **ESC**: Quit

### During Gameplay
- **ESC**: Open pause menu
- Select "Back to Menu" → Return to ROM selection
- Select "Quit" → Exit application

## Advanced Usage

### Custom ROM Metadata

In the future, you'll be able to provide a metadata JSON file:

```bash
chip8recomp --batch roms/ --metadata metadata.json -o output/
```

```json
{
  "pong": {
    "title": "Pong",
    "description": "Classic paddle game",
    "authors": "Paul Vervalin",
    "release": "1990",
    "recommended_cpu_freq": 500
  }
}
```

### Filtering ROMs

To compile only specific ROMs, create a subdirectory:

```bash
mkdir favorites
cp rom1.ch8 rom2.ch8 rom3.ch8 favorites/
chip8recomp --batch favorites/ -o my_favorites
```

## Troubleshooting

### No ROMs Found

**Problem**: "No ROM files found in directory"

**Solution**: Ensure files have `.ch8` or `.chip8` extension

### Build Errors

**Problem**: Undefined references during linking

**Solution**: Check that `CHIP8_RECOMPILED_DIR` in CMakeLists.txt points to the correct path

### Menu Not Appearing

**Problem**: Application starts but no menu is shown

**Solution**: Verify SDL2 and ImGui are properly installed and linked

### Can't Return to Menu

**Problem**: "Back to Menu" option not in pause menu

**Solution**: Ensure you're running a multi-ROM build (not single ROM)

## Performance

- **Compilation Time**: ~1-2 seconds per ROM
- **Menu Rendering**: 60 FPS
- **ROM Switching**: <10ms
- **Memory Usage**: ~100KB per ROM + code size

## Limitations

- **Static Compilation**: ROMs must be compiled into the executable (no dynamic loading)
- **Serial Compilation**: ROMs are compiled one at a time
- **No Hot Reload**: Must rebuild to add/remove ROMs
- **Basic Metadata**: Currently auto-generated from filenames

## Future Enhancements

Planned improvements:

- [ ] JSON metadata file parsing
- [ ] ROM screenshots in menu
- [ ] Per-ROM settings persistence
- [ ] ROM categories and filtering
- [ ] Favorites system
- [ ] Search functionality
- [ ] Parallel compilation
- [ ] Dynamic ROM loading

## See Also

- [MULTI_ROM.md](MULTI_ROM.md) - Comprehensive documentation
- [MULTI_ROM_IMPLEMENTATION.md](MULTI_ROM_IMPLEMENTATION.md) - Implementation details
- [MULTI_ROM_QUICKREF.md](MULTI_ROM_QUICKREF.md) - Quick reference guide

## Questions?

For issues or questions:
1. Check the troubleshooting section above
2. Review the comprehensive documentation in `docs/MULTI_ROM.md`
3. Check existing GitHub issues
4. Open a new issue with details

## License

Same as main project (see LICENSE file).
