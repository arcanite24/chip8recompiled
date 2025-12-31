# CHIP-8 Recompiled: Architecture & Design Document

## Table of Contents

1. [Project Overview](#project-overview)
2. [Goals & Non-Goals](#goals--non-goals)
3. [High-Level Architecture](#high-level-architecture)
4. [Component Deep Dive](#component-deep-dive)
5. [CHIP-8 Instruction Set Analysis](#chip-8-instruction-set-analysis)
6. [Recompiler Design](#recompiler-design)
7. [Runtime Design](#runtime-design)
8. [Code Generation Strategy](#code-generation-strategy)
9. [Build System & Toolchain](#build-system--toolchain)
10. [Platform Abstraction Layer](#platform-abstraction-layer)
11. [Directory Structure](#directory-structure)
12. [Development Phases](#development-phases)
13. [Future Considerations](#future-considerations)

---

## Project Overview

**CHIP-8 Recompiled** is a static recompilation toolchain that transforms CHIP-8 ROM binaries into native executables. Inspired by [N64Recomp](https://github.com/Mr-Wiseguy/N64Recomp), this project aims to provide a modern, portable way to run CHIP-8 programs at native speeds without the overhead of interpretation or dynamic recompilation.

### What is Static Recompilation?

Static recompilation (also known as Ahead-of-Time or AOT recompilation) is the process of translating binary code from one instruction set to another *before* runtime. Unlike emulation (which interprets instructions at runtime) or dynamic recompilation (JIT compilation), static recompilation:

- **Produces standalone executables** that can run without an emulator
- **Enables native compiler optimizations** (the C/C++ compiler can optimize the generated code)
- **Eliminates interpretation overhead** completely
- **Allows platform-specific optimizations** in the runtime layer

### Why CHIP-8?

CHIP-8 is an ideal target for a static recompilation learning project:

1. **Simple instruction set**: Only 35 opcodes (vs. thousands in modern architectures)
2. **Fixed instruction size**: All instructions are 2 bytes
3. **No self-modifying code**: Programs don't typically modify themselves
4. **Small memory model**: 4KB addressable RAM
5. **Well-documented**: Extensive documentation and test ROMs available
6. **Active community**: Many ROMs available for testing

---

## Goals & Non-Goals

### Goals

| Goal | Description |
|------|-------------|
| **Static Recompilation** | Transform CHIP-8 binaries into C source code |
| **Native Execution** | Compile generated C code to native executables |
| **Portability** | Abstract platform-specific code behind clean interfaces |
| **Accuracy** | 100% compatibility with standard CHIP-8 ROMs |
| **Maintainability** | Clean, well-documented codebase |
| **Extensibility** | Easy to add SUPER-CHIP and other variants later |
| **macOS First** | Primary development target is macOS |

### Non-Goals (For Initial Release)

| Non-Goal | Reason |
|----------|--------|
| SUPER-CHIP support | Focus on core CHIP-8 first |
| MegaChip support | Future extension |
| Sound accuracy | Basic beep is sufficient initially |
| GUI configuration | Command-line tool first |
| Live/JIT recompilation | Static-only for simplicity |

---

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           CHIP-8 Recompiled                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌──────────────────────────────────────────────────────────────────────┐   │
│  │                         RECOMPILER (chip8recomp)                      │   │
│  │                                                                       │   │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────────┐   │   │
│  │  │   ROM       │    │ Instruction │    │    Code Generator       │   │   │
│  │  │   Parser    │───▶│  Decoder    │───▶│    (C Emitter)          │   │   │
│  │  └─────────────┘    └─────────────┘    └─────────────────────────┘   │   │
│  │         │                  │                       │                  │   │
│  │         ▼                  ▼                       ▼                  │   │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────────┐   │   │
│  │  │   Control   │    │   Symbol    │    │    Output Files         │   │   │
│  │  │ Flow Graph  │    │   Table     │    │    (.c / .h)            │   │   │
│  │  └─────────────┘    └─────────────┘    └─────────────────────────┘   │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│                                    │                                         │
│                                    │ Generated Code                          │
│                                    ▼                                         │
│  ┌──────────────────────────────────────────────────────────────────────┐   │
│  │                          RUNTIME (libchip8rt)                         │   │
│  │                                                                       │   │
│  │  ┌─────────────────────────────────────────────────────────────────┐ │   │
│  │  │                     Core Runtime Layer                          │ │   │
│  │  │  ┌───────────┐  ┌───────────┐  ┌───────────┐  ┌───────────┐    │ │   │
│  │  │  │  CPU      │  │  Memory   │  │  Timers   │  │  Stack    │    │ │   │
│  │  │  │  Context  │  │  Manager  │  │           │  │  Manager  │    │ │   │
│  │  │  └───────────┘  └───────────┘  └───────────┘  └───────────┘    │ │   │
│  │  └─────────────────────────────────────────────────────────────────┘ │   │
│  │                                                                       │   │
│  │  ┌─────────────────────────────────────────────────────────────────┐ │   │
│  │  │                  Platform Abstraction Layer                     │ │   │
│  │  │  ┌───────────┐  ┌───────────┐  ┌───────────┐  ┌───────────┐    │ │   │
│  │  │  │  Video    │  │  Audio    │  │  Input    │  │  Timing   │    │ │   │
│  │  │  │  Backend  │  │  Backend  │  │  Backend  │  │  Backend  │    │ │   │
│  │  │  └───────────┘  └───────────┘  └───────────┘  └───────────┘    │ │   │
│  │  └─────────────────────────────────────────────────────────────────┘ │   │
│  │                                                                       │   │
│  │  ┌─────────────────────────────────────────────────────────────────┐ │   │
│  │  │                Platform Implementations                         │ │   │
│  │  │  ┌──────────────────────────────────────────────────────────┐  │ │   │
│  │  │  │  macOS (SDL2)  │  Linux (SDL2)  │  Windows (SDL2)  │ ... │  │ │   │
│  │  │  └──────────────────────────────────────────────────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Component Deep Dive

### 1. Recompiler (`chip8recomp`)

The recompiler is a **command-line tool** that takes a CHIP-8 ROM as input and produces C source code as output.

#### Components

| Component | Responsibility |
|-----------|----------------|
| **ROM Parser** | Loads and validates CHIP-8 ROM files |
| **Instruction Decoder** | Decodes 2-byte opcodes into structured instruction data |
| **Control Flow Analyzer** | Identifies functions, jump targets, and basic blocks |
| **Symbol Table** | Tracks labels, addresses, and generated function names |
| **Code Generator** | Emits C code for each instruction |
| **Configuration Parser** | Reads TOML configuration for ROM-specific settings |

#### Input/Output

```
Input:  ROM binary file (.ch8)
        Configuration file (.toml) [optional]

Output: Generated C source files
        Header files with declarations
        Metadata for the runtime
```

### 2. Runtime Library (`libchip8rt`)

The runtime provides all the necessary infrastructure for the recompiled code to execute.

#### Core Runtime Layer

| Component | Responsibility |
|-----------|----------------|
| **CPU Context** | Holds registers (V0-VF, I, PC, SP), flags, and state |
| **Memory Manager** | Manages 4KB RAM, ROM loading, and memory access |
| **Timer System** | Handles delay timer and sound timer (60Hz) |
| **Stack Manager** | Manages 16-level call stack |
| **Instruction Helpers** | Macro implementations for complex operations |

#### Platform Abstraction Layer (PAL)

| Backend | Responsibility |
|---------|----------------|
| **Video Backend** | 64x32 monochrome display rendering |
| **Audio Backend** | Beep sound generation |
| **Input Backend** | 16-key hexadecimal keypad handling |
| **Timing Backend** | High-resolution timing and frame pacing |

---

## CHIP-8 Instruction Set Analysis

### Memory Layout

```
┌─────────────────────────────────────────────────────┐
│ Address Range    │ Size    │ Purpose                │
├─────────────────────────────────────────────────────┤
│ 0x000 - 0x1FF    │ 512B    │ Reserved (Interpreter) │
│ 0x050 - 0x0A0    │ 80B     │ Built-in Font Sprites  │
│ 0x200 - 0xFFF    │ 3584B   │ Program ROM & RAM      │
└─────────────────────────────────────────────────────┘
```

### Registers

| Register | Size | Description |
|----------|------|-------------|
| V0-VF | 8-bit | General purpose (VF = flags) |
| I | 16-bit | Index register |
| PC | 16-bit | Program counter |
| SP | 8-bit | Stack pointer |
| DT | 8-bit | Delay timer |
| ST | 8-bit | Sound timer |

### Complete Instruction Set (35 Opcodes)

| Opcode | Mnemonic | Description | Recompilation Strategy |
|--------|----------|-------------|------------------------|
| `0x00E0` | CLS | Clear display | `runtime_clear_screen(ctx);` |
| `0x00EE` | RET | Return from subroutine | `return;` (end of function) |
| `0x0NNN` | SYS addr | System call (ignored) | No-op or stub |
| `0x1NNN` | JP addr | Jump to address | `goto label_NNN;` or function call |
| `0x2NNN` | CALL addr | Call subroutine | `func_NNN(ctx);` |
| `0x3XNN` | SE Vx, byte | Skip if Vx == NN | `if (ctx->V[X] == NN) { skip; }` |
| `0x4XNN` | SNE Vx, byte | Skip if Vx != NN | `if (ctx->V[X] != NN) { skip; }` |
| `0x5XY0` | SE Vx, Vy | Skip if Vx == Vy | `if (ctx->V[X] == ctx->V[Y]) { skip; }` |
| `0x6XNN` | LD Vx, byte | Set Vx = NN | `ctx->V[X] = NN;` |
| `0x7XNN` | ADD Vx, byte | Add NN to Vx | `ctx->V[X] += NN;` |
| `0x8XY0` | LD Vx, Vy | Set Vx = Vy | `ctx->V[X] = ctx->V[Y];` |
| `0x8XY1` | OR Vx, Vy | Set Vx = Vx OR Vy | `ctx->V[X] \|= ctx->V[Y];` |
| `0x8XY2` | AND Vx, Vy | Set Vx = Vx AND Vy | `ctx->V[X] &= ctx->V[Y];` |
| `0x8XY3` | XOR Vx, Vy | Set Vx = Vx XOR Vy | `ctx->V[X] ^= ctx->V[Y];` |
| `0x8XY4` | ADD Vx, Vy | Add with carry | `ADD_VX_VY(ctx, X, Y);` |
| `0x8XY5` | SUB Vx, Vy | Sub with borrow | `SUB_VX_VY(ctx, X, Y);` |
| `0x8XY6` | SHR Vx | Shift right | `SHR_VX(ctx, X);` |
| `0x8XY7` | SUBN Vx, Vy | Sub reverse | `SUBN_VX_VY(ctx, X, Y);` |
| `0x8XYE` | SHL Vx | Shift left | `SHL_VX(ctx, X);` |
| `0x9XY0` | SNE Vx, Vy | Skip if Vx != Vy | `if (ctx->V[X] != ctx->V[Y]) { skip; }` |
| `0xANNN` | LD I, addr | Set I = NNN | `ctx->I = NNN;` |
| `0xBNNN` | JP V0, addr | Jump to V0 + NNN | Computed jump (complex) |
| `0xCXNN` | RND Vx, byte | Random AND NN | `ctx->V[X] = rand() & NN;` |
| `0xDXYN` | DRW Vx, Vy, n | Draw sprite | `runtime_draw_sprite(ctx, X, Y, N);` |
| `0xEX9E` | SKP Vx | Skip if key pressed | `if (runtime_key_pressed(ctx, X)) { skip; }` |
| `0xEXA1` | SKNP Vx | Skip if key not pressed | `if (!runtime_key_pressed(ctx, X)) { skip; }` |
| `0xFX07` | LD Vx, DT | Get delay timer | `ctx->V[X] = ctx->delay_timer;` |
| `0xFX0A` | LD Vx, K | Wait for key press | `ctx->V[X] = runtime_wait_key(ctx);` |
| `0xFX15` | LD DT, Vx | Set delay timer | `ctx->delay_timer = ctx->V[X];` |
| `0xFX18` | LD ST, Vx | Set sound timer | `ctx->sound_timer = ctx->V[X];` |
| `0xFX1E` | ADD I, Vx | Add Vx to I | `ctx->I += ctx->V[X];` |
| `0xFX29` | LD F, Vx | Set I to font sprite | `ctx->I = FONT_ADDR + ctx->V[X] * 5;` |
| `0xFX33` | LD B, Vx | Store BCD | `runtime_store_bcd(ctx, X);` |
| `0xFX55` | LD [I], Vx | Store V0-Vx | `runtime_store_registers(ctx, X);` |
| `0xFX65` | LD Vx, [I] | Load V0-Vx | `runtime_load_registers(ctx, X);` |

---

## Recompiler Design

### Recompilation Pipeline

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│    Load     │     │   Decode    │     │   Analyze   │     │   Generate  │
│    ROM      │────▶│   Opcodes   │────▶│   Control   │────▶│   C Code    │
│             │     │             │     │    Flow     │     │             │
└─────────────┘     └─────────────┘     └─────────────┘     └─────────────┘
      │                   │                   │                   │
      ▼                   ▼                   ▼                   ▼
 Validate size       Build opcode        Find jump          Emit functions
 Check header        structures          targets            and labels
```

### Control Flow Analysis

The recompiler must identify:

1. **Function Boundaries**: CALL (2NNN) targets define function entry points
2. **Jump Targets**: JP (1NNN) and conditional skips define labels
3. **Basic Blocks**: Sequences of instructions without branches
4. **Computed Jumps**: JP V0 (BNNN) requires special handling

### Handling Computed Jumps (BNNN)

The `JP V0, addr` instruction jumps to `V0 + NNN`, which is data-dependent. Options:

1. **Switch Statement**: Generate a switch over possible V0 values
2. **Function Table**: Use runtime lookup (like N64Recomp's `LOOKUP_FUNC`)
3. **Hybrid**: Analyze data flow to determine possible values

```c
// Option 1: Switch statement
switch (ctx->V[0] + 0x300) {
    case 0x300: goto label_0x300; break;
    case 0x302: goto label_0x302; break;
    // ... known targets
    default: RUNTIME_PANIC("Unknown jump target");
}

// Option 2: Function table lookup
LOOKUP_FUNC(ctx->V[0] + 0x300)(ctx);
```

### Generated Code Structure

```c
// === rom_game.h ===
#ifndef ROM_GAME_H
#define ROM_GAME_H

#include "chip8rt/runtime.h"

void chip8_main(Chip8Context* ctx);
void func_0x200(Chip8Context* ctx);
void func_0x250(Chip8Context* ctx);
// ... more function declarations

#endif

// === rom_game.c ===
#include "rom_game.h"

// Entry point - called from runtime
void chip8_main(Chip8Context* ctx) {
    func_0x200(ctx);
}

// Function at address 0x200
void func_0x200(Chip8Context* ctx) {
    // 0x200: 00E0 - CLS
    runtime_clear_screen(ctx);
    
    // 0x202: 6A00 - LD VA, 0x00
    ctx->V[0xA] = 0x00;
    
    // 0x204: 6B1F - LD VB, 0x1F  
    ctx->V[0xB] = 0x1F;
    
    // 0x206: 2250 - CALL 0x250
    func_0x250(ctx);
    
    // ... more instructions
}

void func_0x250(Chip8Context* ctx) {
    // Subroutine at 0x250
label_0x250:
    // ...
    
    // 0x260: 00EE - RET
    return;
}
```

---

## Runtime Design

### CPU Context Structure

```c
// chip8rt/include/chip8rt/context.h

#ifndef CHIP8RT_CONTEXT_H
#define CHIP8RT_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>

#define CHIP8_MEMORY_SIZE   4096
#define CHIP8_STACK_SIZE    16
#define CHIP8_NUM_REGISTERS 16
#define CHIP8_DISPLAY_WIDTH  64
#define CHIP8_DISPLAY_HEIGHT 32
#define CHIP8_NUM_KEYS      16
#define CHIP8_PROGRAM_START 0x200
#define CHIP8_FONT_START    0x050

typedef struct Chip8Context {
    // Registers
    uint8_t  V[CHIP8_NUM_REGISTERS];  // V0-VF general purpose
    uint16_t I;                        // Index register
    uint16_t PC;                       // Program counter
    uint8_t  SP;                       // Stack pointer
    
    // Timers (decremented at 60Hz)
    uint8_t delay_timer;
    uint8_t sound_timer;
    
    // Memory
    uint8_t memory[CHIP8_MEMORY_SIZE];
    
    // Stack
    uint16_t stack[CHIP8_STACK_SIZE];
    
    // Display (64x32 monochrome)
    uint8_t display[CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT];
    bool    display_dirty;  // Flag for rendering optimization
    
    // Input state
    bool keys[CHIP8_NUM_KEYS];
    
    // Runtime state
    bool     running;
    bool     waiting_for_key;
    uint8_t* key_destination;  // Register to store key when waiting
    
    // Platform callbacks (set by platform layer)
    void* platform_data;
    
} Chip8Context;

// Context lifecycle
Chip8Context* chip8_context_create(void);
void chip8_context_destroy(Chip8Context* ctx);
void chip8_context_reset(Chip8Context* ctx);

#endif
```

### Runtime Macros and Helpers

```c
// chip8rt/include/chip8rt/instructions.h

#ifndef CHIP8RT_INSTRUCTIONS_H
#define CHIP8RT_INSTRUCTIONS_H

#include "context.h"

// === Arithmetic with carry/borrow ===

#define ADD_VX_VY(ctx, x, y) do { \
    uint16_t sum = (uint16_t)(ctx)->V[(x)] + (uint16_t)(ctx)->V[(y)]; \
    (ctx)->V[0xF] = (sum > 255) ? 1 : 0; \
    (ctx)->V[(x)] = (uint8_t)(sum & 0xFF); \
} while(0)

#define SUB_VX_VY(ctx, x, y) do { \
    (ctx)->V[0xF] = ((ctx)->V[(x)] >= (ctx)->V[(y)]) ? 1 : 0; \
    (ctx)->V[(x)] -= (ctx)->V[(y)]; \
} while(0)

#define SUBN_VX_VY(ctx, x, y) do { \
    (ctx)->V[0xF] = ((ctx)->V[(y)] >= (ctx)->V[(x)]) ? 1 : 0; \
    (ctx)->V[(x)] = (ctx)->V[(y)] - (ctx)->V[(x)]; \
} while(0)

#define SHR_VX(ctx, x) do { \
    (ctx)->V[0xF] = (ctx)->V[(x)] & 0x1; \
    (ctx)->V[(x)] >>= 1; \
} while(0)

#define SHL_VX(ctx, x) do { \
    (ctx)->V[0xF] = ((ctx)->V[(x)] & 0x80) >> 7; \
    (ctx)->V[(x)] <<= 1; \
} while(0)

// === Memory Operations ===

static inline uint8_t chip8_read_byte(Chip8Context* ctx, uint16_t addr) {
    return ctx->memory[addr & 0xFFF];
}

static inline void chip8_write_byte(Chip8Context* ctx, uint16_t addr, uint8_t value) {
    ctx->memory[addr & 0xFFF] = value;
}

// === Runtime Functions (implemented in runtime.c) ===

void runtime_clear_screen(Chip8Context* ctx);
bool runtime_draw_sprite(Chip8Context* ctx, uint8_t vx, uint8_t vy, uint8_t height);
bool runtime_key_pressed(Chip8Context* ctx, uint8_t key);
uint8_t runtime_wait_key(Chip8Context* ctx);
void runtime_store_bcd(Chip8Context* ctx, uint8_t x);
void runtime_store_registers(Chip8Context* ctx, uint8_t x);
void runtime_load_registers(Chip8Context* ctx, uint8_t x);
uint8_t runtime_random(void);

// === Timer Management ===

void runtime_tick_timers(Chip8Context* ctx);

#endif
```

### Platform Abstraction Interface

```c
// chip8rt/include/chip8rt/platform.h

#ifndef CHIP8RT_PLATFORM_H
#define CHIP8RT_PLATFORM_H

#include "context.h"

// === Platform Backend Interface ===
// Implement these for each target platform

typedef struct Chip8Platform {
    // Initialization
    bool (*init)(Chip8Context* ctx, const char* title, int scale);
    void (*shutdown)(Chip8Context* ctx);
    
    // Video
    void (*render)(Chip8Context* ctx);
    void (*clear)(Chip8Context* ctx);
    
    // Audio  
    void (*beep_start)(Chip8Context* ctx);
    void (*beep_stop)(Chip8Context* ctx);
    
    // Input
    void (*poll_events)(Chip8Context* ctx);
    bool (*should_quit)(Chip8Context* ctx);
    
    // Timing
    uint64_t (*get_time_us)(void);
    void (*sleep_us)(uint64_t microseconds);
    
} Chip8Platform;

// Register platform implementation
void chip8_set_platform(Chip8Platform* platform);
Chip8Platform* chip8_get_platform(void);

// === Main Loop (provided by runtime) ===

typedef void (*Chip8MainFunc)(Chip8Context* ctx);

// Run the recompiled program with the registered platform
int chip8_run(Chip8MainFunc entry_point, const char* title, int scale);

#endif
```

### SDL2 Platform Implementation

```c
// chip8rt/src/platform_sdl.c

#include "chip8rt/platform.h"
#include <SDL2/SDL.h>

typedef struct SDL_PlatformData {
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;
    SDL_AudioDeviceID audio_device;
    int scale;
    bool quit_requested;
} SDL_PlatformData;

// Key mapping: CHIP-8 hex keypad to keyboard
static const SDL_Scancode KEY_MAP[16] = {
    SDL_SCANCODE_X,    // 0
    SDL_SCANCODE_1,    // 1
    SDL_SCANCODE_2,    // 2
    SDL_SCANCODE_3,    // 3
    SDL_SCANCODE_Q,    // 4
    SDL_SCANCODE_W,    // 5
    SDL_SCANCODE_E,    // 6
    SDL_SCANCODE_A,    // 7
    SDL_SCANCODE_S,    // 8
    SDL_SCANCODE_D,    // 9
    SDL_SCANCODE_Z,    // A
    SDL_SCANCODE_C,    // B
    SDL_SCANCODE_4,    // C
    SDL_SCANCODE_R,    // D
    SDL_SCANCODE_F,    // E
    SDL_SCANCODE_V     // F
};

static bool sdl_init(Chip8Context* ctx, const char* title, int scale) {
    // Implementation...
}

static void sdl_render(Chip8Context* ctx) {
    // Implementation...
}

// ... more implementations

static Chip8Platform sdl_platform = {
    .init        = sdl_init,
    .shutdown    = sdl_shutdown,
    .render      = sdl_render,
    .clear       = sdl_clear,
    .beep_start  = sdl_beep_start,
    .beep_stop   = sdl_beep_stop,
    .poll_events = sdl_poll_events,
    .should_quit = sdl_should_quit,
    .get_time_us = sdl_get_time_us,
    .sleep_us    = sdl_sleep_us,
};

Chip8Platform* chip8_platform_sdl(void) {
    return &sdl_platform;
}
```

---

## Code Generation Strategy

### Translation Rules

Following N64Recomp's approach, each CHIP-8 instruction is translated literally into C code:

```c
// Example: Translating a simple program

// CHIP-8 bytecode at 0x200:
// 00E0      CLS
// 6A05      LD VA, 5
// 6B0A      LD VB, 10
// 8AB4      ADD VA, VB (with carry)
// 1200      JP 0x200 (loop)

// Generated C code:
void func_0x200(Chip8Context* ctx) {
label_0x200:
    // 0x200: 00E0 - CLS
    runtime_clear_screen(ctx);
    
    // 0x202: 6A05 - LD VA, 0x05
    ctx->V[0xA] = 0x05;
    
    // 0x204: 6B0A - LD VB, 0x0A
    ctx->V[0xB] = 0x0A;
    
    // 0x206: 8AB4 - ADD VA, VB
    ADD_VX_VY(ctx, 0xA, 0xB);
    
    // 0x208: 1200 - JP 0x200
    goto label_0x200;
}
```

### Skip Instructions

Skip instructions (3XNN, 4XNN, 5XY0, 9XY0, EX9E, EXA1) skip the next instruction if a condition is true:

```c
// 0x200: 3A05 - SE VA, 0x05 (skip if VA == 5)
// 0x202: 6B00 - LD VB, 0x00
// 0x204: 6C01 - LD VC, 0x01

// Generated:
if (ctx->V[0xA] == 0x05) {
    goto skip_0x204;  // Skip next instruction
}
// 0x202: 6B00
ctx->V[0xB] = 0x00;

skip_0x204:
// 0x204: 6C01
ctx->V[0xC] = 0x01;
```

### Subroutine Handling

```c
// CALL instruction becomes a function call
// 0x200: 2300 - CALL 0x300

func_0x300(ctx);  // Direct function call

// RET instruction ends the function
// 0x310: 00EE - RET

return;  // End of func_0x300
```

---

## Build System & Toolchain

### CMake Structure

```cmake
# CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.20)
project(chip8-recompiled VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Options
option(CHIP8_BUILD_RECOMPILER "Build the recompiler tool" ON)
option(CHIP8_BUILD_RUNTIME "Build the runtime library" ON)
option(CHIP8_BUILD_TESTS "Build test programs" ON)

# Dependencies
find_package(SDL2 REQUIRED)

# Subdirectories
if(CHIP8_BUILD_RECOMPILER)
    add_subdirectory(recompiler)
endif()

if(CHIP8_BUILD_RUNTIME)
    add_subdirectory(runtime)
endif()

if(CHIP8_BUILD_TESTS)
    add_subdirectory(tests)
endif()
```

### Build Workflow

```bash
# 1. Build the toolchain
mkdir build && cd build
cmake -G Ninja ..
cmake --build .

# 2. Recompile a ROM
./chip8recomp ../roms/games/Pong.ch8 -o ../output/pong

# 3. Build the native executable
cd ../output/pong
mkdir build && cd build
cmake -G Ninja ..
cmake --build .

# 4. Run!
./pong
```

### Generated Project Structure

```
output/pong/
├── CMakeLists.txt          # Generated build file
├── pong.c                  # Generated code
├── pong.h                  # Generated header
├── rom_data.c              # Embedded ROM data (for sprites, etc.)
└── main.c                  # Entry point (uses runtime)
```

---

## Platform Abstraction Layer

### Abstraction Goals

1. **Zero platform code in generated files**: All platform-specific code lives in the runtime
2. **Callback-based I/O**: Platform provides callbacks, runtime uses them
3. **Easy porting**: Implement one struct to port to a new platform

### Supported Platforms (Planned)

| Platform | Backend | Status |
|----------|---------|--------|
| macOS | SDL2 | Primary target |
| Linux | SDL2 | Future |
| Windows | SDL2 | Future |
| Web | Emscripten + SDL2 | Future |
| Embedded | Custom | Future |

---

## Directory Structure

```
chip8-recompiled/
├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── docs/
│   ├── ARCHITECTURE.md          # This document
│   ├── CHIP8_SPEC.md             # CHIP-8 specification reference
│   └── BUILDING.md               # Build instructions
│
├── recompiler/                   # chip8recomp tool
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── recompiler/
│   │       ├── decoder.h         # Instruction decoder
│   │       ├── analyzer.h        # Control flow analyzer
│   │       ├── generator.h       # Code generator
│   │       ├── config.h          # Configuration parser
│   │       └── rom.h             # ROM loader
│   └── src/
│       ├── main.cpp              # Entry point
│       ├── decoder.cpp
│       ├── analyzer.cpp
│       ├── generator.cpp
│       ├── config.cpp
│       └── rom.cpp
│
├── runtime/                      # libchip8rt
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── chip8rt/
│   │       ├── context.h         # CPU context
│   │       ├── instructions.h    # Instruction macros
│   │       ├── runtime.h         # Main runtime header
│   │       └── platform.h        # Platform abstraction
│   └── src/
│       ├── context.c
│       ├── runtime.c
│       ├── platform_sdl.c        # SDL2 implementation
│       └── font.c                # Built-in font data
│
├── templates/                    # Code generation templates
│   ├── CMakeLists.txt.in         # Template for generated projects
│   └── main.c.in                 # Template for entry point
│
├── roms/                         # Test ROMs (existing)
│   ├── demos/
│   ├── games/
│   └── programs/
│
├── tests/                        # Test suite
│   ├── CMakeLists.txt
│   ├── test_decoder.cpp
│   ├── test_generator.cpp
│   └── integration/
│       └── test_roms.cpp
│
└── examples/                     # Example recompiled games
    └── pong/
        └── ...
```

---

## Development Phases

### Phase 1: Foundation (Weeks 1-2)

- [ ] Project setup (CMake, directory structure)
- [ ] ROM loader implementation
- [ ] Instruction decoder (all 35 opcodes)
- [ ] Basic code generator (simple straight-line code)
- [ ] CPU context structure
- [ ] Basic runtime (no I/O)

### Phase 2: Control Flow (Weeks 3-4)

- [ ] Control flow analyzer
- [ ] Jump target identification
- [ ] Function boundary detection
- [ ] Label generation
- [ ] Subroutine handling (CALL/RET)

### Phase 3: Runtime I/O (Weeks 5-6)

- [ ] SDL2 platform backend
- [ ] Display rendering
- [ ] Input handling
- [ ] Timer system (60Hz)
- [ ] Basic audio (beep)

### Phase 4: Integration (Weeks 7-8)

- [ ] End-to-end testing with real ROMs
- [ ] Bug fixes and compatibility
- [ ] Performance optimization
- [ ] Documentation

### Phase 5: Polish (Weeks 9+)

- [ ] TOML configuration support
- [ ] Debug output mode
- [ ] More platform backends
- [ ] SUPER-CHIP support (optional)

---

## Future Considerations

### SUPER-CHIP Support

SUPER-CHIP adds:
- 128x64 high-resolution mode
- 16x16 sprites
- Additional opcodes
- Scroll instructions

The architecture should accommodate these extensions with minimal changes.

### Live Recompilation

Following N64Recomp's LiveRecomp, future versions could support:
- Runtime code patching
- Mod support
- Debug features

### Web Export

Using Emscripten, the same codebase could target WebAssembly for browser execution.

### Multiple Backend Support

The code generator could emit:
- C (current)
- C++ (alternative)
- Rust (alternative)
- LLVM IR (direct to native)

---

## References

1. [N64Recomp](https://github.com/Mr-Wiseguy/N64Recomp) - Primary inspiration
2. [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime) - Runtime architecture reference
3. [Cowgod's CHIP-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
4. [CHIP-8 Wikipedia](https://en.wikipedia.org/wiki/CHIP-8)
5. [jamulator](https://github.com/andrewrk/jamulator) - NES static recompiler

---

## Appendix A: Example TOML Configuration

```toml
# pong.toml - Configuration for Pong ROM

[rom]
path = "roms/games/Pong.ch8"
name = "Pong"

[output]
directory = "output/pong"
prefix = "pong"

[recompiler]
# Generate debug comments in output
debug_comments = true

# Emit timing checkpoints for debugging
timing_checkpoints = false

[functions]
# Override automatic function detection
# entry_points = [0x200, 0x250, 0x300]

[quirks]
# CHIP-8 quirk modes (for compatibility)
# shift_uses_vy = false      # SHR/SHL use VY (original) vs VX (modern)
# load_store_increments_i = true  # FX55/FX65 increment I
# jump_uses_v0 = true        # BNNN uses V0 (vs VX in SUPER-CHIP)
```

---

## Appendix B: Opcode Decoding Reference

```c
// Opcode structure (big-endian, 2 bytes)
// Most significant nibble determines instruction class

uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];

uint8_t  op   = (opcode & 0xF000) >> 12;  // First nibble
uint8_t  x    = (opcode & 0x0F00) >> 8;   // Second nibble (register)
uint8_t  y    = (opcode & 0x00F0) >> 4;   // Third nibble (register)  
uint8_t  n    = (opcode & 0x000F);        // Fourth nibble
uint8_t  nn   = (opcode & 0x00FF);        // Lower byte
uint16_t nnn  = (opcode & 0x0FFF);        // Lower 12 bits (address)
```

---

*Document Version: 1.0*  
*Last Updated: December 30, 2025*  
*Authors: CHIP-8 Recompiled Team*
