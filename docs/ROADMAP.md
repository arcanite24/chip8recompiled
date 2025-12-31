# CHIP-8 Recompiled - Development Roadmap

## Current Status: v0.4.0 (Phase 3 Complete - Dear ImGui Integration)

### ✅ Completed Features
- [x] Static recompilation of CHIP-8 ROMs to C code
- [x] SDL2 runtime with graphics, audio, and input
- [x] 35 instruction decoder
- [x] Control flow analysis (functions, basic blocks)
- [x] Cooperative yielding for frame pacing
- [x] Cycle-accurate timing
- [x] Key repeat rate limiting
- [x] CMake-based build system
- [x] Single-function mode for complex ROMs
- [x] On-the-fly decoding for odd address targets
- [x] Reachability-based code generation
- [x] Automated ROM compatibility testing (100% pass rate)
- [x] Valid C identifier generation (handles numeric prefixes)
- [x] **In-game pause menu** (ESC to open)
- [x] **Graphics settings** (scale, fullscreen, color themes)
- [x] **Audio settings** (volume, frequency, waveform, mute)
- [x] **Gameplay settings** (CPU speed, key repeat, quirks)
- [x] **Config file save/load** (INI format, per-ROM settings)
- [x] **Window size presets** (1x, 2x, 5x, 10x, 15x, 20x, Custom)
- [x] **Dear ImGui integration** (debug overlay, settings UI)
- [x] **FPS counter** (F1 to toggle)
- [x] **Debug overlay** (F2 to toggle - registers, memory, disassembly)
- [x] **ImGui settings panel** (F3 to toggle)
- [x] **CRT scanline effect**
- [x] **Pixel grid overlay**

### ✅ Working Games
- All 199 tested ROMs (82% compatibility - 99% for CHIP-8 only)
- Pong, Breakout, Tetris, Space Invaders, Blinky, 15 Puzzle, and more

---

## Phase 1: Full ROM Compatibility (v0.2.0) ✅ COMPLETE

### ✅ Priority: HIGH - COMPLETED

**Goal**: Fix Space Invaders and achieve 100% ROM compatibility

#### Analyzer Improvements
- [x] **Better function detection**: Detect all CALL targets, even in data sections
- [x] **Cross-function jumps**: Handle JPs that go outside current function
- [x] **Data vs code discrimination**: Detect sprite data vs executable code via reachability
- [ ] **Computed jump analysis**: Better handling of BNNN (JP V0, addr) *(partial)*

#### Code Generation Fixes
- [x] **Single-function mode**: `--single-function` flag generates all code in one function
- [x] **On-the-fly decoding**: Decode at any address (even/odd) during reachability analysis
- [x] **Fall-through blocks**: Ensure all reachable code is generated
- [x] **Label resolution**: Generate labels for all jump targets
- [x] **CALL/RET in single function**: Software stack + switch dispatch for returns

#### Testing
- [x] Space Invaders working with --single-function
- [x] Create automated test suite
- [x] Test all ROMs in /roms directory
- [x] Track compatibility percentage (100%)

---

## Phase 1.5: Tooling & Optimization Improvements (v0.2.5)

### 🟡 Priority: HIGH

**Goal**: Improve build tooling, test infrastructure, and generated code quality

#### Test Script Enhancements
- [ ] **Extended ROM support**: Handle hi-res ROMs in `roms/hires/` directory
- [x] **Parallel test execution**: Build multiple ROMs concurrently using `xargs -P`
- [x] **Better progress reporting**: Show real-time status with ETA
- [x] **Configurable test sets**: Allow testing subsets of ROMs (all, classic, archive, chip8only, quick)
- [x] **chip8Archive integration**: Added 100+ ROMs from chip8Archive to test suite
- [ ] **Performance benchmarks**: Track build times per ROM

#### Compiler Optimizations for Generated Code
- [ ] **Aggressive optimization flags**: Enable `-O3` in generated CMakeLists.txt
- [ ] **Link-Time Optimization (LTO)**: Add `-flto` for cross-module inlining
- [ ] **Profile-Guided Optimization (PGO)**: Generate optimized builds using runtime profiles
- [ ] **Computed goto dispatch**: Replace switch statement with GCC/Clang computed goto
- [ ] **Inline runtime functions**: Mark hot paths as `static inline`
- [ ] **Branch prediction hints**: Use `__builtin_expect` for common paths

#### Analyzer Improvements to Reduce Single-Function Mode Dependency
- [ ] **Improved data detection**: Use heuristics to detect sprite/font data in code sections
- [ ] **Pattern-based analysis**: Detect common ROM header patterns (ASCII art, metadata)
- [ ] **Call graph refinement**: Better cross-function jump target resolution  
- [ ] **Conservative vs aggressive mode**: Let user choose analysis strictness
- [ ] **Self-modifying code detection**: Flag ROMs that may modify code at runtime

#### Build System
- [ ] **Precompiled runtime**: Build libchip8rt once, link statically
- [ ] **Incremental builds**: Only rebuild changed ROMs in test suite
- [ ] **Build cache**: Cache successful builds for identical ROM hashes

---

## Phase 2: Runtime Settings & Options (v0.3.0) ✅ COMPLETE

### ✅ Priority: HIGH - COMPLETED

**Goal**: Add N64Recomp-style runtime options and settings

#### In-Game Menu
- [x] **Pause menu** (press ESC)
  - Resume
  - Settings submenus
  - Reset game
  - Quit confirmation

#### Graphics Settings
- [x] Window scaling (1x to 20x)
- [x] Window size presets (1x, 2x, 5x, 10x, 15x, 20x, Custom)
- [x] Fullscreen toggle
- [x] Color themes (Classic, Green Phosphor, Amber, LCD, Custom)
- [x] CRT scanline effect
- [x] Pixel grid overlay setting

#### Audio Settings
- [x] Volume control (0-100%)
- [x] Beep frequency (220Hz - 880Hz)
- [x] Waveform selection (Square, Sine, Triangle, Sawtooth, Noise)
- [x] Mute toggle

#### Gameplay Settings
- [x] CPU speed slider (100Hz - 2000Hz)
- [x] Key repeat delay/rate
- [ ] Custom key bindings *deferred to Phase 3.5*
- [x] Quirk toggles:
  - [x] VF reset for AND/OR/XOR
  - [x] Shift uses VY
  - [x] Load/Store increments I
  - [x] Wrap sprites at screen edges
  - [x] Jump uses VX (SCHIP)
  - [x] Display wait (VBLANK sync)

#### Save/Load
- [x] Save settings to INI config file
- [x] Load settings from config file
- [x] Per-game settings (saved at ~/.chip8recompiled/games/<rom>.ini)
- [ ] Save states *deferred to Phase 5*

---

## Phase 3: Enhanced UI (v0.4.0) ✅ COMPLETE

### ✅ Priority: MEDIUM - COMPLETED

**Goal**: Polished user interface with Dear ImGui

#### Implementation
- [x] Integrate Dear ImGui with SDL2
- [x] Debug overlay (registers, memory, disassembly)
- [x] FPS counter and performance stats
- [x] ROM information display

#### Hotkeys
- F1: Toggle FPS counter
- F2: Toggle debug overlay (registers, stack, keys, disassembly, memory)
- F3: Toggle settings window
- F10: Toggle entire overlay
- ESC: Open pause menu (existing)

#### Debug Overlay Features
- Register display (V0-VF, I, PC, DT, ST, SP)
- Stack viewer
- Keypad state visualization
- Live disassembly around PC
- Memory viewer with address navigation
- FPS graph history

---

## Phase 3.5: Deferred Features (v0.4.5)

### 🟡 Priority: MEDIUM

**Goal**: Implement features deferred from Phase 2

#### Custom Key Bindings
- [ ] Key binding UI in settings
- [ ] Save key bindings to config
- [ ] Per-ROM key configurations

#### CRT Shader Effect (Advanced)
- [ ] OpenGL/Vulkan shader support
- [ ] Screen curvature effect
- [ ] Phosphor glow simulation
- [ ] Color bleeding effect

---

## Phase 4: SUPER-CHIP & XO-CHIP Support (v0.5.0)

### 🟢 Priority: LOW

**Goal**: Support extended CHIP-8 variants

#### SUPER-CHIP (SCHIP)
- [ ] 128x64 high-resolution mode
- [ ] Scroll instructions (00CN, 00FB, 00FC)
- [ ] Exit instruction (00FD)
- [ ] Extended font (16x16)
- [ ] HP48 flag storage (FX75, FX85)

#### XO-CHIP
- [ ] 4 display planes (colors)
- [ ] Audio with custom patterns
- [ ] Extended memory (64KB)
- [ ] Plane selection instruction

---

## Phase 5: Multi-Platform & Distribution (v0.6.0)

### 🟢 Priority: MEDIUM

**Goal**: Port to other platforms and distribute compiled binaries

#### Distribution & CI/CD
- [ ] **GitHub Releases**: Publish compiled recompiler binaries
  - [ ] macOS (arm64, x86_64)
  - [ ] Linux (x86_64, AppImage)
  - [ ] Windows (x64, installer)
- [ ] **GitHub Actions**: Automated builds on push/release
- [ ] **Homebrew formula**: `brew install chip8recomp`
- [ ] **Docker image**: Containerized recompiler for CI pipelines

#### WebAssembly Target
- [ ] **Emscripten backend**: Compile recompiled ROMs to WebAssembly
- [ ] **Browser runtime**: SDL2 → Emscripten/HTML5 Canvas
- [ ] **Web player**: Online CHIP-8 player with ROM selection
- [ ] **Progressive Web App**: Installable web version

#### Android Target
- [ ] **Android NDK build**: Cross-compile runtime for Android
- [ ] **SDL2 Android**: Use SDL2's Android backend
- [ ] **Touch controls**: On-screen keypad for touch input
- [ ] **Android app template**: APK generation from recompiled ROM

#### Other Targets
- [ ] **Windows**: MSVC build, installer
- [ ] **Linux**: AppImage, Flatpak
- [ ] **iOS**: SDL2 iOS backend (limited distribution)
- [ ] **Embedded**: Raspberry Pi, ESP32

#### Platform Abstraction
- [ ] Abstract SDL2 behind interface
- [ ] Add alternative backends (GLFW, Raylib)
- [ ] Minimal embedded runtime (framebuffer only)

---

## Phase 6: Developer Tools (v0.7.0)

### 🟢 Priority: LOW

**Goal**: Tools for game developers

#### Debugger
- [ ] Breakpoints
- [ ] Step-through execution
- [ ] Memory watch
- [ ] Register editing
- [ ] Disassembly view

#### ROM Analysis
- [ ] Decompiler output viewer
- [ ] Call graph visualization
- [ ] Data section detection
- [ ] ROM statistics

---

## Technical Debt & Cleanup

### Analysis of Single-Function Mode Usage

Currently, 55 of 145 standard CHIP-8 ROMs (38%) require `--single-function` mode. Root causes:

| Issue | ROMs Affected | Potential Fix |
|-------|---------------|---------------|
| ASCII art headers | ~10 | Detect non-instruction byte patterns |
| Data in code section | ~8 | Improved reachability with data markers |
| Computed jumps (BNNN) | ~3 | Better V0+addr target estimation |
| Cross-function control flow | ~1 | Merge connected function groups |

**Goal**: Reduce single-function mode to <10% of ROMs through improved static analysis.

### ROM Library Expansion
- [x] **chip8Archive integration**: Add support for ROMs from [JohnEarnest/chip8Archive](https://github.com/JohnEarnest/chip8Archive)
  - [x] Integrated 100+ ROMs into test suite
  - [x] Parse platform metadata to filter SCHIP/XO-CHIP
  - [ ] Parse quirk settings from programs.json
  - [ ] Auto-configure quirks per ROM
- [ ] **Find CC0/permissive ROMs**: Curate collection of freely distributable games
  - [ ] Document license for each ROM
  - [ ] Include sample games in repository
- [ ] **Homebrew showcase**: Feature modern homebrew games
  - [ ] Octojam entries
  - [ ] Revival Studios games
  - [ ] Community submissions

### Code Generation Improvements (Low Priority)
- [ ] **Template-based generation**: Replace string concatenation with template engine
  - [ ] Consider Mustache, Jinja2, or custom template format
  - [ ] Separate code templates from generator logic
- [ ] **LLVM IR generation**: Alternative backend generating LLVM IR instead of C
  - [ ] Enables direct optimization without C compiler
  - [ ] Better control over generated assembly
- [ ] **Language-agnostic output**: Support multiple target languages
  - [ ] Rust output for memory safety
  - [ ] JavaScript/TypeScript for web
  - [ ] Go for simplicity

### Code Quality Tasks
- [ ] Add unit tests for decoder
- [ ] Add integration tests for runtime
- [ ] Refactor analyzer for clarity
- [ ] Add comprehensive documentation
- [ ] Performance profiling
- [ ] Memory leak detection
- [ ] Code coverage reporting

---

## Contributing

See CONTRIBUTING.md for guidelines on:
- Code style
- Pull request process
- Issue reporting
- Feature requests
