# CHIP-8 Recompiled

A static recompilation toolchain for CHIP-8 ROMs that transforms bytecode into native executables.

## Overview

**CHIP-8 Recompiled** is a static recompiler that transforms CHIP-8 ROM binaries into native C code, which can then be compiled into standalone executables. Unlike interpreters, recompiled programs run natively with zero interpreter overhead.

### Key Components

| Component | Description |
|-----------|-------------|
| **`chip8recomp`** | The recompiler - converts CHIP-8 bytecode into C source code |
| **`libchip8rt`** | The runtime library - provides platform abstraction and CHIP-8 system emulation |

Inspired by [N64Recomp](https://github.com/Mr-Wiseguy/N64Recomp), this project brings static recompilation techniques to the CHIP-8 platform.

## Architecture

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   CHIP-8 ROM    │────▶│   chip8recomp   │────▶│   C Source      │
│   (.ch8)        │     │   (recompiler)  │     │   Files         │
└─────────────────┘     └─────────────────┘     └─────────────────┘
                                                        │
                                                        ▼
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Native        │◀────│   C Compiler    │◀────│   libchip8rt    │
│   Executable    │     │   (clang/gcc)   │     │   (runtime)     │
└─────────────────┘     └─────────────────┘     └─────────────────┘
```

## Quick Start

### Prerequisites

- CMake 3.20+
- C11/C++20 compiler (clang recommended)
- SDL2
- Ninja (recommended)

### Building

```bash
# Clone the repository
git clone https://github.com/yourusername/chip8-recompiled.git
cd chip8-recompiled

# Build the toolchain
mkdir build && cd build
cmake -G Ninja ..
cmake --build .
```

### Recompiling a ROM

```bash
# Standard recompilation (works for most ROMs)
./recompiler/chip8recomp ../roms/games/Pong\ \[Paul\ Vervalin,\ 1990\].ch8 -o pong_output

# For complex ROMs with data sections mixed with code
./recompiler/chip8recomp ../roms/games/Space\ Invaders\ \[David\ Winter\].ch8 -o invaders_output --single-function

# Build and run the native executable
cd pong_output
mkdir build && cd build
cmake -G Ninja ..
cmake --build .
./pong
```

### Controls

| CHIP-8 Key | Keyboard |
|------------|----------|
| 1 2 3 C    | 1 2 3 4  |
| 4 5 6 D    | Q W E R  |
| 7 8 9 E    | A S D F  |
| A 0 B F    | Z X C V  |

### System Keys

| Key | Action |
|-----|--------|
| ESC | Open pause menu / Back |
| F1 | Toggle FPS counter |
| F2 | Toggle debug overlay |
| F3 | Toggle settings window |
| F10 | Toggle entire ImGui overlay |

## Runtime Features

### Debug Overlay (F2)

The debug overlay provides real-time insight into the CHIP-8 emulation:

- **Registers**: V0-VF, I, PC, DT (delay timer), ST (sound timer), SP
- **Stack**: Current call stack contents
- **Keypad**: Visual keypad state indicator
- **Disassembly**: Live disassembly around current PC
- **Memory Viewer**: Browse memory with current instruction highlighting

### Settings Window (F3)

Adjust emulation settings in real-time:

- **Graphics**: Window size, colors, pixel grid, CRT effect
- **Audio**: Volume, tone frequency, mute
- **Gameplay**: CPU speed, quirk toggles
- **Overlay**: Show/hide individual overlay components

### Configuration

Settings are automatically saved to:
- **Global**: `~/.chip8recompiled/settings.ini`
- **Per-ROM**: `~/.chip8recompiled/games/<rom_name>.ini`

## Compatibility

**82% ROM compatibility** (165/199 ROMs tested) - 99% for CHIP-8 only (145/146)

| Status | Count | Description |
|--------|-------|-------------|
| ✅ Normal Mode | 123 | Standard recompilation |
| ✅ Single-Function | 42 | Requires `--single-function` flag |
| ⚠️ SCHIP/XO-CHIP | 34 | Requires extended support (planned) |

### Tested Games (Highlights)

| Game | Status | Mode |
|------|--------|------|
| Pong | ✅ Works | Normal |
| Breakout | ✅ Works | Normal |
| Tetris | ✅ Works | Normal |
| Space Invaders | ✅ Works | Single-function |
| Blinky (Pac-Man) | ✅ Works | Single-function |
| Connect 4 | ✅ Works | Normal |
| 15 Puzzle | ✅ Works | Single-function |

See [COMPATIBILITY_REPORT.md](COMPATIBILITY_REPORT.md) for the full list.

### When to Use Single-Function Mode

Use `--single-function` for ROMs that have:
- ASCII art headers or data mixed with code
- Jumps to odd addresses
- Complex cross-function control flow

```bash
./recompiler/chip8recomp rom.ch8 -o output --single-function
```

## Project Structure

```
chip8-recompiled/
├── recompiler/          # chip8recomp - ROM to C translator
│   ├── src/             # Decoder, analyzer, generator
│   └── include/         # Public headers
├── runtime/             # libchip8rt - Runtime library
│   ├── src/             # Context, instructions, platform
│   └── include/         # Public headers
├── scripts/             # Build & test scripts
│   └── test_roms.sh     # Compatibility test suite
├── docs/                # Documentation
│   ├── ARCHITECTURE.md  # Design document
│   └── ROADMAP.md       # Development roadmap
├── roms/                # Test ROMs
│   ├── games/           # Game ROMs
│   └── demos/           # Demo ROMs
└── build/               # Build output
```

## Features

### Recompiler (`chip8recomp`)

- ✅ Full CHIP-8 instruction set (35 opcodes)
- ✅ Control flow analysis (functions, basic blocks)
- ✅ Reachability-based code generation
- ✅ Two compilation modes (normal & single-function)
- ✅ On-the-fly decoding for odd address targets
- ✅ Cooperative yielding for backward jumps
- ✅ Embedded ROM data for sprites

### Runtime (`libchip8rt`)

- ✅ SDL2 backend (graphics, audio, input)
- ✅ 60Hz timer system
- ✅ Configurable CPU frequency (100-2000Hz)
- ✅ Key repeat rate limiting
- ✅ Platform abstraction layer
- ✅ **In-game pause menu** (ESC to open)
- ✅ **Runtime settings** (graphics, audio, gameplay)
- ✅ **Color themes** (Classic, Green Phosphor, Amber, LCD)
- ✅ **Audio waveforms** (Square, Sine, Triangle, Sawtooth, Noise)
- ✅ **Quirk toggles** for ROM compatibility
- ✅ **Config file support** (save/load settings)

## Testing

Run the compatibility test suite:

```bash
./scripts/test_roms.sh
```

This tests all ROMs in the `roms/` directory and generates `COMPATIBILITY_REPORT.md`.

## Supported Platforms

| Platform | Status |
|----------|--------|
| macOS    | ✅ Fully Supported |
| Linux    | ✅ Should Work (SDL2) |
| Windows  | 🔄 Planned |

### New in v0.3.0: In-Game Settings Menu

Press **ESC** during gameplay to open the pause menu:

| Feature | Options |
|---------|---------|
| **Graphics** | Scale (1-20x), Fullscreen, Color Themes, Pixel Grid |
| **Audio** | Volume, Frequency (220-880Hz), Waveform, Mute |
| **Gameplay** | CPU Speed, Key Repeat, Quirk Toggles |
| **Color Themes** | Classic, Green Phosphor, Amber, LCD, Custom |

### Upcoming Improvements

- [ ] **chip8Archive Support** - Integrate ROMs from [JohnEarnest/chip8Archive](https://github.com/JohnEarnest/chip8Archive)
- [ ] **Prebuilt Binaries** - Publish compiled recompiler for macOS, Linux, Windows
- [ ] **WebAssembly Target** - Run recompiled ROMs in the browser
- [ ] **Android Target** - Mobile support with touch controls
- [ ] **Expanded ROM Testing** - Hi-res ROMs, parallel builds
- [ ] **Compiler Optimizations** - LTO, `-O3`, computed goto

## License

MIT License - See [LICENSE](LICENSE) for details.

## Acknowledgments

- [N64Recomp](https://github.com/Mr-Wiseguy/N64Recomp) - Inspiration and architecture reference
- [Cowgod's CHIP-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
- [chip8Archive](https://github.com/JohnEarnest/chip8Archive) - Curated ROM collection
