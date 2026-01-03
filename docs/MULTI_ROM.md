# Multi-ROM Runtime Documentation

This document describes the multi-ROM launcher feature that allows batch recompilation of multiple CHIP-8 ROMs into a single executable with an integrated ROM selection menu.

## Overview

The multi-ROM runtime enables you to:
1. Batch compile multiple CHIP-8 ROMs at once
2. Generate a single executable that includes all ROMs
3. Present a visual ROM selection menu using ImGui
4. Switch between ROMs during runtime via the pause menu
5. Maintain separate settings for each ROM

## Usage

### Batch Compilation

To compile multiple ROMs into a single launcher:

```bash
chip8recomp --batch path/to/roms/ -o multi_rom_output
```

This will:
- Scan the directory for all `.ch8` and `.chip8` files
- Compile each ROM individually
- Generate a unified launcher with ROM selection menu
- Create a single `CMakeLists.txt` for the entire project

### Optional: Metadata File

You can provide additional ROM metadata via a JSON file:

```bash
chip8recomp --batch roms/ --metadata metadata.json -o output/
```

The metadata file format (future enhancement):
```json
{
  "pong": {
    "title": "Pong",
    "description": "Classic arcade game",
    "authors": "Paul Vervalin",
    "release": "1990",
    "recommended_cpu_freq": 500
  }
}
```

### Building the Multi-ROM Launcher

After batch compilation:

```bash
cd multi_rom_output
mkdir build && cd build
cmake -G Ninja ..
cmake --build .
./chip8_launcher
```

## Architecture

### Components

#### 1. ROM Catalog (`runtime/include/chip8rt/rom_catalog.h`)

Defines the `RomEntry` structure that contains:
- ROM identifier and title
- Embedded ROM data
- Entry point function
- Function registration callback
- Optional metadata (description, authors, etc.)

#### 2. ROM Selector (`runtime/src/rom_selector.cpp`)

ImGui-based ROM selection menu that:
- Displays a list of available ROMs
- Shows ROM metadata on hover
- Handles ROM launching and menu navigation
- Manages context switching between ROMs

#### 3. Batch Compiler (`recompiler/src/batch.cpp`)

Orchestrates multi-ROM compilation:
- Scans directory for ROMs
- Compiles each ROM with unique namespace prefix
- Generates unified catalog and launcher files
- Creates consolidated CMakeLists.txt

#### 4. Menu Integration (`runtime/src/menu.c`)

Enhanced pause menu with:
- "Back to Menu" option (when in multi-ROM mode)
- Confirmation dialogs for menu navigation
- Proper cleanup and state management

### Generated Files

For a batch compilation, the following files are generated:

```
output/
├── rom1.c                  # Compiled ROM 1
├── rom1.h                  # ROM 1 header
├── rom1_rom_data.c         # ROM 1 embedded data
├── rom2.c                  # Compiled ROM 2
├── rom2.h                  # ROM 2 header
├── rom2_rom_data.c         # ROM 2 embedded data
├── ...
├── rom_catalog.c           # Catalog of all ROMs
├── main.c                  # Launcher entry point
└── CMakeLists.txt          # Build configuration
```

## Runtime Behavior

### Initial Launch

1. Application starts with ROM selection menu
2. User navigates with arrow keys or mouse
3. Clicking or pressing ENTER launches selected ROM

### During Gameplay

1. Press ESC to open pause menu
2. Navigate to "Back to Menu" option
3. Confirm to return to ROM selection
4. Select a different ROM or quit

### Context Switching

When switching between ROMs:
1. Current ROM stops execution
2. Context is reset (registers, memory, etc.)
3. Function table is cleared
4. New ROM's functions are registered
5. New ROM is loaded and executed

## Implementation Details

### Function Table Management

Each ROM registers its functions at unique addresses. When switching ROMs:
- `chip8_clear_function_table()` zeros the lookup table
- New ROM's `<rom>_register_functions()` populates it
- Computed jumps (`BNNN`) work correctly per-ROM

### Memory Isolation

Each ROM has:
- Its own embedded ROM data
- Separate entry point
- Isolated function namespace (via prefixing)
- Individual settings (future enhancement)

### Multi-ROM Mode Flag

The runtime uses `chip8_menu_set_multi_rom_mode(true)` to:
- Add "Back to Menu" to the pause menu
- Enable ROM switching logic
- Handle confirmation dialogs appropriately

## Example Workflow

### 1. Prepare ROMs

```bash
mkdir my_games
cp pong.ch8 tetris.ch8 space_invaders.ch8 my_games/
```

### 2. Batch Compile

```bash
chip8recomp --batch my_games -o game_collection
```

### 3. Build

```bash
cd game_collection
mkdir build && cd build
cmake -G Ninja ..
cmake --build .
```

### 4. Run

```bash
./chip8_launcher
```

## Configuration Options

### Generator Options

All standard generator options work in batch mode:

- `--no-comments`: Disable disassembly comments
- `--single-function`: Use single-function mode for complex ROMs
- `--debug`: Enable debug output

Example:
```bash
chip8recomp --batch roms/ --single-function --no-comments -o output/
```

## Future Enhancements

Potential improvements:

1. **JSON Metadata Parsing**: Full support for external metadata files
2. **ROM Categories**: Group ROMs by type/genre in the menu
3. **Favorites**: Mark frequently played ROMs
4. **Search/Filter**: Quick ROM lookup in large collections
5. **Screenshots**: Display ROM screenshots in selection menu
6. **Per-ROM Settings**: Remember settings per-ROM
7. **Hot Reload**: Reload ROM without restart

## Troubleshooting

### Issue: ROMs not found during batch compilation

**Solution**: Ensure ROM files have `.ch8` or `.chip8` extension

### Issue: Build errors about missing symbols

**Solution**: Ensure the `CHIP8_RECOMPILED_DIR` path in CMakeLists.txt is correct

### Issue: Menu doesn't appear

**Solution**: Verify ImGui initialization and SDL2 renderer setup

### Issue: Can't return to menu from ROM

**Solution**: Check that `chip8_menu_set_multi_rom_mode(true)` is called in main

## Technical Notes

### Why Separate ROM Data?

Each ROM embeds its own data for:
- Sprite data access during execution
- Memory operations that reference ROM area
- BCD conversion and font handling

### Context Reset

Full context reset between ROMs includes:
- V registers cleared
- I register cleared
- Program counter reset
- Stack cleared
- Timers reset
- Display cleared
- Keys released

### Thread Safety

The current implementation is single-threaded. ROM switching happens synchronously when requested via the menu.

## API Reference

### Multi-ROM Functions

```c
// Run multi-ROM launcher
int chip8_run_with_menu(const RomEntry* catalog, size_t count);

// Clear function lookup table
void chip8_clear_function_table(void);

// Set multi-ROM mode in menu
void chip8_menu_set_multi_rom_mode(bool enabled);

// Request return to menu (called by pause menu)
void chip8_request_return_to_menu(void);
```

### RomEntry Structure

```c
typedef struct RomEntry {
    const char* name;                    // Unique identifier
    const char* title;                   // Display title
    const uint8_t* data;                 // ROM data
    size_t size;                         // ROM size
    Chip8EntryPoint entry;               // Entry function
    void (*register_functions)(void);    // Function registration
    int recommended_cpu_freq;            // Suggested CPU speed
    const char* description;             // Optional description
    const char* authors;                 // Optional authors
    const char* release;                 // Optional release date
} RomEntry;
```

## Performance Considerations

### Compilation Time

Batch compilation is serial (one ROM at a time). For large collections:
- Expect ~1-2 seconds per ROM
- Complex ROMs take longer (more functions)
- Use `--no-comments` to speed up generation

### Runtime Performance

ROM switching is fast:
- Context reset: <1ms
- Function registration: ~1-5ms depending on ROM size
- No performance impact during gameplay

### Memory Usage

Each ROM contributes:
- Compiled code (varies by ROM complexity)
- Embedded ROM data (typical: 512 - 4KB)
- Catalog entry: ~100 bytes

A collection of 50 ROMs typically uses <5MB total.

## License & Credits

This multi-ROM feature is part of the chip8-recompiled project.
See the main LICENSE file for details.
