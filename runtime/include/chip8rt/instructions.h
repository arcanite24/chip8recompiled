/**
 * @file instructions.h
 * @brief CHIP-8 instruction helper macros and functions
 * 
 * This header provides macros that implement CHIP-8 instruction semantics.
 * These are called from recompiled code to perform operations like arithmetic
 * with carry/borrow flags, sprite drawing, etc.
 */

#ifndef CHIP8RT_INSTRUCTIONS_H
#define CHIP8RT_INSTRUCTIONS_H

#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Arithmetic Macros with Flag Handling
 * ========================================================================== */

/**
 * @brief ADD Vx, Vy - Add with carry flag (8XY4)
 * 
 * Vx = Vx + Vy
 * VF = 1 if overflow (result > 255), 0 otherwise
 * 
 * Note: When x == 0xF, the flag result must survive (not the math result).
 * We save the sum first, then set VF last.
 */
#define CHIP8_ADD_VX_VY(ctx, x, y) do { \
    uint16_t _sum = (uint16_t)(ctx)->V[(x)] + (uint16_t)(ctx)->V[(y)]; \
    (ctx)->V[(x)] = (uint8_t)(_sum & 0xFF); \
    (ctx)->V[0xF] = (_sum > 255) ? 1 : 0; \
} while(0)

/**
 * @brief SUB Vx, Vy - Subtract with borrow flag (8XY5)
 * 
 * Vx = Vx - Vy
 * VF = 1 if Vx >= Vy (NOT borrow), 0 otherwise
 * 
 * Note: When x == 0xF, the flag result must survive.
 * We save inputs first, compute result, then set VF last.
 */
#define CHIP8_SUB_VX_VY(ctx, x, y) do { \
    uint8_t _vx = (ctx)->V[(x)]; \
    uint8_t _vy = (ctx)->V[(y)]; \
    (ctx)->V[(x)] = _vx - _vy; \
    (ctx)->V[0xF] = (_vx >= _vy) ? 1 : 0; \
} while(0)

/**
 * @brief SUBN Vx, Vy - Subtract reverse with borrow flag (8XY7)
 * 
 * Vx = Vy - Vx
 * VF = 1 if Vy >= Vx (NOT borrow), 0 otherwise
 * 
 * Note: When x == 0xF, the flag result must survive.
 */
#define CHIP8_SUBN_VX_VY(ctx, x, y) do { \
    uint8_t _vx = (ctx)->V[(x)]; \
    uint8_t _vy = (ctx)->V[(y)]; \
    (ctx)->V[(x)] = _vy - _vx; \
    (ctx)->V[0xF] = (_vy >= _vx) ? 1 : 0; \
} while(0)

/**
 * @brief SHR Vx - Shift right (8XY6)
 * 
 * VF = least significant bit of Vx before shift
 * Vx = Vx >> 1
 * 
 * Note: Original CHIP-8 stored Vy >> 1 in Vx. Modern interpreters
 * just shift Vx. This uses modern behavior by default.
 * When x == 0xF, the flag result must survive.
 */
#define CHIP8_SHR_VX(ctx, x) do { \
    uint8_t _vx = (ctx)->V[(x)]; \
    (ctx)->V[(x)] = _vx >> 1; \
    (ctx)->V[0xF] = _vx & 0x01; \
} while(0)

/**
 * @brief SHR Vx, Vy - Shift right with source (8XY6, original behavior)
 * 
 * VF = least significant bit of Vy
 * Vx = Vy >> 1
 * 
 * When x == 0xF, the flag result must survive.
 */
#define CHIP8_SHR_VX_VY(ctx, x, y) do { \
    uint8_t _vy = (ctx)->V[(y)]; \
    (ctx)->V[(x)] = _vy >> 1; \
    (ctx)->V[0xF] = _vy & 0x01; \
} while(0)

/**
 * @brief SHL Vx - Shift left (8XYE)
 * 
 * VF = most significant bit of Vx before shift
 * Vx = Vx << 1
 * 
 * When x == 0xF, the flag result must survive.
 */
#define CHIP8_SHL_VX(ctx, x) do { \
    uint8_t _vx = (ctx)->V[(x)]; \
    (ctx)->V[(x)] = _vx << 1; \
    (ctx)->V[0xF] = (_vx & 0x80) >> 7; \
} while(0)

/**
 * @brief SHL Vx, Vy - Shift left with source (8XYE, original behavior)
 * 
 * VF = most significant bit of Vy
 * Vx = Vy << 1
 * 
 * When x == 0xF, the flag result must survive.
 */
#define CHIP8_SHL_VX_VY(ctx, x, y) do { \
    uint8_t _vy = (ctx)->V[(y)]; \
    (ctx)->V[(x)] = _vy << 1; \
    (ctx)->V[0xF] = (_vy & 0x80) >> 7; \
} while(0)

/* ============================================================================
 * Memory Access Helpers
 * ========================================================================== */

/**
 * @brief Read a byte from memory
 * 
 * @param ctx CHIP-8 context
 * @param addr Memory address (masked to 12 bits)
 * @return Byte value at address
 */
static inline uint8_t chip8_read_byte(Chip8Context* ctx, uint16_t addr) {
    return ctx->memory[addr & 0x0FFF];
}

/**
 * @brief Write a byte to memory
 * 
 * @param ctx CHIP-8 context
 * @param addr Memory address (masked to 12 bits)
 * @param value Byte value to write
 */
static inline void chip8_write_byte(Chip8Context* ctx, uint16_t addr, uint8_t value) {
    ctx->memory[addr & 0x0FFF] = value;
}

/**
 * @brief Read a 16-bit word from memory (big-endian)
 * 
 * @param ctx CHIP-8 context
 * @param addr Memory address
 * @return 16-bit value (big-endian decoded)
 */
static inline uint16_t chip8_read_word(Chip8Context* ctx, uint16_t addr) {
    uint16_t masked = addr & 0x0FFF;
    return ((uint16_t)ctx->memory[masked] << 8) | ctx->memory[masked + 1];
}

/* ============================================================================
 * Runtime Functions (Implemented in runtime.c)
 * ========================================================================== */

/**
 * @brief CLS - Clear the display (00E0)
 * 
 * @param ctx CHIP-8 context
 */
void chip8_clear_screen(Chip8Context* ctx);

/**
 * @brief DRW Vx, Vy, N - Draw sprite (DXYN)
 * 
 * Draws an N-byte sprite from memory location I at position (Vx, Vy).
 * Sprites are XORed onto the display. VF is set to 1 if any pixels
 * are erased (collision detection).
 * 
 * @param ctx CHIP-8 context
 * @param vx X coordinate register index
 * @param vy Y coordinate register index
 * @param height Sprite height in bytes (1-15)
 */
void chip8_draw_sprite(Chip8Context* ctx, uint8_t vx, uint8_t vy, uint8_t height);

/**
 * @brief SKP Vx / SKNP Vx - Check key state (EX9E, EXA1)
 * 
 * @param ctx CHIP-8 context
 * @param key Key index (0x0-0xF)
 * @return true if key is currently pressed
 */
bool chip8_key_pressed(Chip8Context* ctx, uint8_t key);

/**
 * @brief LD Vx, K - Wait for key press (FX0A)
 * 
 * Blocks execution until a key is pressed and released.
 * Returns the key value (0x0-0xF).
 * 
 * @param ctx CHIP-8 context
 * @param reg Register index to store key value
 */
void chip8_wait_key(Chip8Context* ctx, uint8_t reg);

/**
 * @brief LD B, Vx - Store BCD representation (FX33)
 * 
 * Stores the BCD (Binary-Coded Decimal) representation of Vx
 * in memory at I, I+1, I+2 (hundreds, tens, ones).
 * 
 * @param ctx CHIP-8 context
 * @param x Register index containing value to convert
 */
void chip8_store_bcd(Chip8Context* ctx, uint8_t x);

/**
 * @brief LD [I], Vx - Store registers V0-Vx in memory (FX55)
 * 
 * Stores registers V0 through Vx in memory starting at address I.
 * 
 * @param ctx CHIP-8 context
 * @param x Last register to store (0x0-0xF)
 * @param increment_i If true, I is set to I + x + 1 after (original behavior)
 */
void chip8_store_registers(Chip8Context* ctx, uint8_t x, bool increment_i);

/**
 * @brief LD Vx, [I] - Load registers V0-Vx from memory (FX65)
 * 
 * Loads registers V0 through Vx from memory starting at address I.
 * 
 * @param ctx CHIP-8 context
 * @param x Last register to load (0x0-0xF)
 * @param increment_i If true, I is set to I + x + 1 after (original behavior)
 */
void chip8_load_registers(Chip8Context* ctx, uint8_t x, bool increment_i);

/**
 * @brief RND Vx, NN - Generate random number (CXNN)
 * 
 * @return Random byte (0-255)
 */
uint8_t chip8_random_byte(void);

/**
 * @brief Seed the random number generator
 * 
 * @param seed Seed value (typically time-based)
 */
void chip8_random_seed(uint32_t seed);

/* ============================================================================
 * Timer Functions
 * ========================================================================== */

/**
 * @brief Decrement timers (called at 60Hz)
 * 
 * Decrements delay_timer and sound_timer if they are non-zero.
 * 
 * @param ctx CHIP-8 context
 */
void chip8_tick_timers(Chip8Context* ctx);

/**
 * @brief Check if sound should be playing
 * 
 * @param ctx CHIP-8 context
 * @return true if sound_timer > 0
 */
static inline bool chip8_sound_active(Chip8Context* ctx) {
    return ctx->sound_timer > 0;
}

/* ============================================================================
 * Yielding Support for Cooperative Multitasking
 * ========================================================================== */

/**
 * @brief Yield macro for cooperative multitasking
 * 
 * This is called after each instruction in the main game loop.
 * It decrements the cycle counter and yields back to the runtime
 * when the frame's worth of cycles is exhausted.
 * 
 * Usage in generated code:
 *   label_0x22A:
 *       ... instruction code ...
 *       CHIP8_YIELD(ctx, 0x22A);
 *       ... next instruction ...
 */
#define CHIP8_YIELD(ctx, resume_addr) do { \
    if (--(ctx)->cycles_remaining <= 0) { \
        (ctx)->resume_pc = (resume_addr); \
        (ctx)->should_yield = true; \
        return; \
    } \
} while(0)

/**
 * @brief Check if we should resume from a previous yield
 * 
 * Usage at the start of a function with a loop:
 *   void func_0x200(Chip8Context* ctx) {
 *       CHIP8_RESUME_CHECK(ctx, label_0x22A, 0x22A);
 *       ... code ...
 *   label_0x22A:
 *       ... loop code ...
 *   }
 */
#define CHIP8_RESUME_CHECK(ctx, label, addr) do { \
    if ((ctx)->should_yield && (ctx)->resume_pc == (addr)) { \
        (ctx)->should_yield = false; \
        goto label; \
    } \
} while(0)

#ifdef __cplusplus
}
#endif

#endif /* CHIP8RT_INSTRUCTIONS_H */
