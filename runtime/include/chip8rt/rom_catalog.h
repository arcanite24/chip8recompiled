/**
 * @file rom_catalog.h
 * @brief ROM catalog structure for multi-ROM launcher
 * 
 * Defines structures for storing metadata about multiple ROMs
 * that have been batch-compiled into a single executable.
 */

#ifndef CHIP8RT_ROM_CATALOG_H
#define CHIP8RT_ROM_CATALOG_H

#include "runtime.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ROM Catalog Entry
 * ========================================================================== */

/**
 * @brief Single ROM entry in the catalog
 * 
 * Contains all information needed to run a specific ROM from
 * a batch-compiled multi-ROM executable.
 */
typedef struct RomEntry {
    /** Unique identifier for the ROM (filename without extension) */
    const char* name;
    
    /** Human-readable title for display */
    const char* title;
    
    /** Pointer to embedded ROM data (for sprites, etc.) */
    const uint8_t* data;
    
    /** Size of ROM data in bytes */
    size_t size;
    
    /** Entry point function for this ROM */
    Chip8EntryPoint entry;
    
    /** Function to register all functions for computed jumps */
    void (*register_functions)(void);
    
    /** Recommended CPU frequency in Hz (0 = use default) */
    int recommended_cpu_freq;
    
    /** Optional: Short description */
    const char* description;
    
    /** Optional: Author(s) */
    const char* authors;
    
    /** Optional: Release date */
    const char* release;
} RomEntry;

/* ============================================================================
 * Multi-ROM Launcher
 * ========================================================================== */

/**
 * @brief Run a multi-ROM launcher with selection menu
 * 
 * Displays a ROM selection menu using ImGui, then runs the selected
 * ROM. Supports returning to the menu via the pause menu.
 * 
 * @param catalog Array of ROM entries
 * @param count Number of ROMs in catalog
 * @return 0 on success, non-zero on error
 */
int chip8_run_with_menu(const RomEntry* catalog, size_t count);

/**
 * @brief Clear the function table
 * 
 * Zeros out the function lookup table to prepare for loading
 * a different ROM's functions.
 */
void chip8_clear_function_table(void);

#ifdef __cplusplus
}
#endif

#endif /* CHIP8RT_ROM_CATALOG_H */
