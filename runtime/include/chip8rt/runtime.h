/**
 * @file runtime.h
 * @brief Main CHIP-8 runtime header
 * 
 * This is the primary header to include in recompiled CHIP-8 programs.
 * It includes all necessary runtime components.
 */

#ifndef CHIP8RT_RUNTIME_H
#define CHIP8RT_RUNTIME_H

/* Core runtime components */
#include "context.h"
#include "instructions.h"
#include "platform.h"
#include "settings.h"
#include "menu.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Version Information
 * ========================================================================== */

#define CHIP8RT_VERSION_MAJOR 0
#define CHIP8RT_VERSION_MINOR 4
#define CHIP8RT_VERSION_PATCH 0

#define CHIP8RT_VERSION_STRING "0.4.0"

/**
 * @brief Get the runtime version string
 * 
 * @return Version string (e.g., "0.1.0")
 */
const char* chip8rt_version(void);

/* ============================================================================
 * Utility Macros for Generated Code
 * ========================================================================== */

/**
 * @brief Mark a location as unreachable
 * 
 * Used for computed jumps that should never reach the default case.
 */
#if defined(__GNUC__) || defined(__clang__)
    #define CHIP8_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
    #define CHIP8_UNREACHABLE() __assume(0)
#else
    #define CHIP8_UNREACHABLE() ((void)0)
#endif

/**
 * @brief Panic and halt execution
 * 
 * Used for unrecoverable errors in recompiled code.
 */
void chip8_panic(const char* message, uint16_t address);

/**
 * @brief Log a debug message
 * 
 * Only outputs if debug mode is enabled.
 */
void chip8_debug(const char* format, ...);

/* ============================================================================
 * Function Lookup (for computed jumps)
 * ========================================================================== */

/**
 * @brief Function lookup table entry
 * 
 * Used for BNNN (JP V0, addr) computed jumps.
 */
typedef void (*Chip8FuncPtr)(Chip8Context* ctx);

/**
 * @brief Register a function at an address
 * 
 * Called during initialization to build the function lookup table.
 * 
 * @param address CHIP-8 address (0x200-0xFFF)
 * @param func Function pointer
 */
void chip8_register_function(uint16_t address, Chip8FuncPtr func);

/**
 * @brief Look up a function by address
 * 
 * Used for computed jumps. Returns NULL if no function at address.
 * 
 * @param address CHIP-8 address
 * @return Function pointer, or NULL
 */
Chip8FuncPtr chip8_lookup_function(uint16_t address);

/**
 * @brief Macro for computed jump (BNNN)
 * 
 * Looks up and calls a function based on computed address.
 */
#define CHIP8_COMPUTED_JUMP(ctx, base_addr) do { \
    uint16_t _target = (base_addr) + (ctx)->V[0]; \
    Chip8FuncPtr _func = chip8_lookup_function(_target); \
    if (_func) { \
        _func(ctx); \
    } else { \
        chip8_panic("Invalid computed jump target", _target); \
    } \
} while(0)

#ifdef __cplusplus
}
#endif

#endif /* CHIP8RT_RUNTIME_H */
