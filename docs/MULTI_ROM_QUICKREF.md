# Multi-ROM Quick Reference

## Command Line

### Batch Compile ROMs
```bash
chip8recomp --batch <rom_directory> -o <output_directory>
```

### With Options
```bash
chip8recomp --batch roms/ -o output/ --no-comments --single-function
```

### With Metadata (future)
```bash
chip8recomp --batch roms/ --metadata metadata.json -o output/
```

## Building Multi-ROM Executable

```bash
cd output_directory
mkdir build && cd build
cmake -G Ninja ..
cmake --build .
./chip8_launcher
```

## Key Code Paths

### Batch Compilation Flow
1. `main.cpp`: Parse `--batch` option
2. `batch.cpp:compile_batch()`: Orchestrate compilation
3. For each ROM:
   - Load and validate ROM
   - Decode instructions
   - Analyze control flow
   - Generate C code with unique prefix
   - Write `.c`, `.h`, `_rom_data.c`
4. Generate `rom_catalog.c` with all ROM entries
5. Generate unified `main.c` and `CMakeLists.txt`

### Runtime Launcher Flow
1. `main.c`: Call `chip8_run_with_menu()`
2. `rom_selector.cpp`: Display ImGui menu
3. User selects ROM
4. Clear function table
5. Register ROM's functions
6. Call `chip8_run()` with ROM entry point
7. If "Back to Menu" selected:
   - Set `g_return_to_menu = true`
   - Return to step 2

### Menu "Back to Menu" Flow
1. User presses ESC → Opens pause menu
2. Navigate to "Back to Menu"
3. Confirm → Sets `menu.menu_requested = true`
4. `runtime.c` checks flag → Calls `chip8_request_return_to_menu()`
5. ROM execution stops
6. Returns to ROM selection menu

## Important Functions

### Multi-ROM Launcher
```c
// Launch multi-ROM menu and run selected ROMs
int chip8_run_with_menu(const RomEntry* catalog, size_t count);
```

### Context Management
```c
// Clear all registered functions
void chip8_clear_function_table(void);

// Enable multi-ROM mode in pause menu
void chip8_menu_set_multi_rom_mode(bool enabled);

// Signal return to ROM menu
void chip8_request_return_to_menu(void);
```

### Batch Compilation
```c++
namespace chip8recomp {
    int compile_batch(const BatchOptions& options);
}
```

## Generated File Structure

```
output/
├── pong.c                    # Compiled ROM: Pong
├── pong.h
├── pong_rom_data.c
├── tetris.c                  # Compiled ROM: Tetris
├── tetris.h
├── tetris_rom_data.c
├── invaders.c                # Compiled ROM: Space Invaders
├── invaders.h
├── invaders_rom_data.c
├── rom_catalog.c             # Catalog of all 3 ROMs
├── main.c                    # Multi-ROM launcher entry
└── CMakeLists.txt            # Build config for all ROMs
```

## ROM Catalog Structure

```c
extern const uint8_t pong_rom_data[];
extern void pong_entry(Chip8Context* ctx);
extern void pong_register_functions(void);

const RomEntry rom_catalog[] = {
    {
        .name = "pong",
        .title = "Pong",
        .data = pong_rom_data,
        .size = sizeof(pong_rom_data),
        .entry = pong_entry,
        .register_functions = pong_register_functions,
        .recommended_cpu_freq = 500,
        .description = "Classic arcade game",
        .authors = "Paul Vervalin",
        .release = "1990"
    },
    // ... more ROMs
};

const size_t rom_catalog_count = 3;
```

## Multi-ROM main.c Template

```c
#include <chip8rt/rom_catalog.h>
#include <chip8rt/platform.h>
#include <chip8rt/menu.h>

extern const RomEntry rom_catalog[];
extern const size_t rom_catalog_count;

int main(int argc, char* argv[]) {
    chip8_set_platform(chip8_platform_sdl2());
    chip8_menu_set_multi_rom_mode(true);
    return chip8_run_with_menu(rom_catalog, rom_catalog_count);
}
```

## Adding Custom Metadata

Currently auto-generated from filename. Future support for JSON:

```json
{
  "pong": {
    "title": "Pong",
    "description": "The classic arcade game",
    "authors": "Paul Vervalin",
    "release": "1990",
    "recommended_cpu_freq": 500
  }
}
```

## Common Issues & Solutions

### Issue: Batch compilation fails to find ROMs
**Solution**: Ensure files have `.ch8` or `.chip8` extension

### Issue: Undefined reference errors during build
**Solution**: Check `CHIP8_RECOMPILED_DIR` path in CMakeLists.txt

### Issue: ROM menu doesn't appear
**Solution**: Verify `chip8_menu_set_multi_rom_mode(true)` is called

### Issue: Can't return to menu from ROM
**Solution**: Ensure multi-ROM mode is enabled

### Issue: Crash when switching ROMs
**Solution**: Check that function table is cleared before re-registration

## Testing Checklist

- [ ] Batch compile 2-3 test ROMs
- [ ] Build without errors
- [ ] Run launcher and see ROM menu
- [ ] Select first ROM → plays correctly
- [ ] Press ESC → pause menu appears
- [ ] Select "Back to Menu" → returns to ROM list
- [ ] Select different ROM → plays correctly
- [ ] Repeat switching several times → no crashes
- [ ] Quit from ROM → application exits cleanly

## Performance Notes

- **Compilation**: ~1-2 seconds per ROM
- **ROM Switching**: <10ms
- **Menu FPS**: 60 FPS
- **Memory per ROM**: ~100KB + code size

## Integration Points

### With Existing Runtime
- Uses existing `chip8_run()` function
- Leverages existing ImGui integration
- Works with current pause menu system
- Compatible with settings system

### With Existing Recompiler
- Reuses ROM loading and validation
- Uses same decoder and analyzer
- Generates same C code structure
- Just adds batching layer

## Files Modified Summary

**Runtime**:
- `runtime.h`, `runtime.c` - Function table clearing
- `menu.h`, `menu.c` - Multi-ROM mode support
- Added `rom_catalog.h` - Catalog structure
- Added `rom_selector.cpp` - Menu implementation

**Recompiler**:
- `main.cpp` - Batch mode option parsing
- Added `batch.h`, `batch.cpp` - Batch compilation

**Documentation**:
- `MULTI_ROM.md` - User guide
- `MULTI_ROM_IMPLEMENTATION.md` - Developer guide

## Next Steps for Development

1. Implement JSON metadata parsing
2. Add ROM screenshots/preview
3. Per-ROM settings persistence
4. ROM categories and filtering
5. Favorites system
6. Better error messages
7. Progress indicators for batch compilation
8. Parallel ROM compilation
