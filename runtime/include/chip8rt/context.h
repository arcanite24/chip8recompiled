/**
 * @file context.h
 * @brief CHIP-8 CPU context and state definitions
 * 
 * This header defines the core CHIP-8 machine state including registers,
 * memory, display, and runtime flags.
 */

#ifndef CHIP8RT_CONTEXT_H
#define CHIP8RT_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Constants
 * ========================================================================== */

/** Total addressable memory (4KB) */
#define CHIP8_MEMORY_SIZE       4096

/** Maximum stack depth for subroutines */
#define CHIP8_STACK_SIZE        16

/** Number of general-purpose registers (V0-VF) */
#define CHIP8_NUM_REGISTERS     16

/** Display width in pixels */
#define CHIP8_DISPLAY_WIDTH     64

/** Display height in pixels */
#define CHIP8_DISPLAY_HEIGHT    32

/** Total display size in pixels */
#define CHIP8_DISPLAY_SIZE      (CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT)

/** Number of keys on the hex keypad */
#define CHIP8_NUM_KEYS          16

/** Program start address (after interpreter area) */
#define CHIP8_PROGRAM_START     0x200

/** Built-in font start address */
#define CHIP8_FONT_START        0x050

/** Size of each font character sprite (5 bytes) */
#define CHIP8_FONT_CHAR_SIZE    5

/** Number of font characters (0-F) */
#define CHIP8_FONT_NUM_CHARS    16

/** Timer frequency in Hz */
#define CHIP8_TIMER_FREQ_HZ     60

/** Target CPU cycles per second (approximate) */
#define CHIP8_CPU_FREQ_HZ       700

/* ============================================================================
 * CPU Context Structure
 * ========================================================================== */

/**
 * @brief CHIP-8 machine state context
 * 
 * Contains all CPU registers, memory, display buffer, and runtime state.
 * This structure is passed to all recompiled functions.
 */
typedef struct Chip8Context {
    /* === Registers === */
    
    /** General-purpose registers V0-VF (VF is the flag register) */
    uint8_t V[CHIP8_NUM_REGISTERS];
    
    /** Index register (12-bit, used for memory addresses) */
    uint16_t I;
    
    /** Program counter (unused in recompiled code, kept for debugging) */
    uint16_t PC;
    
    /** Stack pointer (0-15) */
    uint8_t SP;
    
    /* === Timers === */
    
    /** Delay timer - decremented at 60Hz, read/write accessible */
    uint8_t delay_timer;
    
    /** Sound timer - decremented at 60Hz, beep when > 0 */
    uint8_t sound_timer;
    
    /* === Memory === */
    
    /** Main memory (4KB) - contains font, program, and working RAM */
    uint8_t memory[CHIP8_MEMORY_SIZE];
    
    /** Call stack for subroutine return addresses */
    uint16_t stack[CHIP8_STACK_SIZE];
    
    /* === Display === */
    
    /** 
     * Display buffer (64x32 monochrome)
     * 0 = pixel off (black), non-zero = pixel on (white)
     * Indexed as: display[y * CHIP8_DISPLAY_WIDTH + x]
     */
    uint8_t display[CHIP8_DISPLAY_SIZE];
    
    /** Flag indicating display needs to be redrawn */
    bool display_dirty;
    
    /* === Input === */
    
    /** Current key state (true = pressed) for keys 0x0-0xF */
    bool keys[CHIP8_NUM_KEYS];
    
    /** Previous frame key state (for edge detection) */
    bool keys_prev[CHIP8_NUM_KEYS];
    
    /** Key that was just released (for FX0A wait instruction) */
    int8_t last_key_released;
    
    /* === Runtime State === */
    
    /** Flag indicating the program should continue running */
    bool running;
    
    /** Flag indicating CPU is blocked waiting for a key press (FX0A) */
    bool waiting_for_key;
    
    /** Register index to store key value when waiting */
    uint8_t key_wait_register;
    
    /* === Yielding Support === */
    
    /** Cycles remaining in current frame (for cooperative yielding) */
    int cycles_remaining;
    
    /** Program counter to resume from after yield */
    uint16_t resume_pc;
    
    /** Flag indicating we should yield back to main loop */
    bool should_yield;
    
    /* === Platform Data === */
    
    /** Opaque pointer to platform-specific data (SDL window, etc.) */
    void* platform_data;
    
    /* === Debug/Statistics === */
    
    /** Total instructions executed (for debugging) */
    uint64_t instruction_count;
    
    /** Current frame number */
    uint64_t frame_count;
    
} Chip8Context;

/* ============================================================================
 * Context Lifecycle Functions
 * ========================================================================== */

/**
 * @brief Create and initialize a new CHIP-8 context
 * 
 * Allocates memory and initializes all fields to their default state.
 * The built-in font is loaded into memory at CHIP8_FONT_START.
 * 
 * @return Pointer to new context, or NULL on allocation failure
 */
Chip8Context* chip8_context_create(void);

/**
 * @brief Destroy a CHIP-8 context and free resources
 * 
 * @param ctx Context to destroy (safe to pass NULL)
 */
void chip8_context_destroy(Chip8Context* ctx);

/**
 * @brief Reset context to initial state
 * 
 * Clears registers, display, and stack. Preserves loaded program in memory.
 * 
 * @param ctx Context to reset
 */
void chip8_context_reset(Chip8Context* ctx);

/**
 * @brief Load program data into context memory
 * 
 * Copies program bytes into memory starting at CHIP8_PROGRAM_START (0x200).
 * 
 * @param ctx Context to load into
 * @param program_data Pointer to program bytes
 * @param size Size of program in bytes
 * @return true on success, false if program too large
 */
bool chip8_context_load_program(Chip8Context* ctx, 
                                 const uint8_t* program_data, 
                                 size_t size);

#ifdef __cplusplus
}
#endif

#endif /* CHIP8RT_CONTEXT_H */
