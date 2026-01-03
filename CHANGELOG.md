# Changelog

All notable changes to CHIP-8 Recompiled will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.8.0] - 2026-01-02

### Added

- **Batch Compilation Mode** - Compile entire ROM directories into a single multi-ROM launcher
  - `--batch <directory>` flag to process all .ch8 files in a directory
  - Automatic ROM catalog generation with metadata from chip8Archive
  - Prefixed symbols for safe multi-ROM linking

- **Multi-ROM Launcher** - Native executable containing multiple CHIP-8 games
  - ImGui-based ROM selection menu with keyboard and mouse navigation
  - Arrow keys to navigate, Enter/Space to launch, Escape to quit
  - Auto-scrolling list keeps selection visible

- **chip8Archive Integration** - Works with [JohnEarnest/chip8Archive](https://github.com/JohnEarnest/chip8Archive)
  - Reads `programs.json` for ROM metadata (title, author, description)
  - Recommended CPU frequencies per game
  - Quirk settings from archive

- **Back to Menu** - Return to ROM selector from in-game pause menu
  - New "Back to Menu" button in Settings overlay (ESC key)
  - Only appears in multi-ROM launcher mode

- **Computed Jump Support** - JP V0,addr (BNNN opcode) in single-function mode
  - Inline switch statement for computed jump targets
  - Fixes panic in games like Bowling

### Changed

- **Auto Mode for Batch** - Batch compilation always uses single-function mode for maximum compatibility
- **Default Scale** - Changed default window scale from 10x to 20x (1280x640)
- **Settings Menu** - Added Resume, Reset Game, Back to Menu, and Quit buttons

### Fixed

- ROM selector UI no longer has overlapping items
- Keyboard navigation moves one item at a time (fixed duplicate input handling)
- Mouse selection works correctly without conflicting hover behavior

## [0.3.0] - 2025-12-15

### Added

- **In-Game Pause Menu** - Press ESC during gameplay
  - Resume, Reset, Settings access
  - Visual overlay with game state preserved

- **Settings Window** - Real-time configuration
  - Graphics: Scale, fullscreen, color themes, pixel grid
  - Audio: Volume, frequency, waveform selection, mute
  - Gameplay: CPU speed, key repeat, quirk toggles

- **Color Themes** - Multiple display themes
  - Classic (white on black)
  - Green Phosphor (retro monitor)
  - Amber (vintage terminal)
  - LCD (gray shades)
  - Custom RGB colors

- **Audio Waveforms** - Sound customization
  - Square, Sine, Triangle, Sawtooth, Noise

- **Configuration Persistence**
  - Global settings: `~/.chip8recompiled/settings.ini`
  - Per-ROM settings: `~/.chip8recompiled/games/<rom>.ini`

## [0.2.0] - 2025-12-01

### Added

- **Debug Overlay** (F2) - Real-time debugging
  - Register display (V0-VF, I, PC, timers)
  - Stack visualization
  - Keypad state indicator
  - Live disassembly
  - Memory viewer

- **Single-Function Mode** - `--single-function` flag for complex ROMs
  - Handles data mixed with code
  - Supports odd-address jumps
  - Uses computed goto for control flow

### Fixed

- Improved ROM compatibility to 82% (165/199 ROMs)

## [0.1.0] - 2025-11-15

### Added

- Initial release
- Static recompilation of CHIP-8 ROMs to C code
- Full CHIP-8 instruction set (35 opcodes)
- SDL2 runtime with graphics, audio, and input
- Control flow analysis (functions, basic blocks)
- Standard keyboard mapping (QWERTY to CHIP-8 hex keypad)
