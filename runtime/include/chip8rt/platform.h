/**
 * @file platform.h
 * @brief Platform abstraction layer for CHIP-8 runtime
 * 
 * This header defines the interface that platform backends must implement.
 * The abstraction allows the same recompiled code to run on different
 * platforms (SDL2, web, embedded, etc.) by swapping the backend.
 */

#ifndef CHIP8RT_PLATFORM_H
#define CHIP8RT_PLATFORM_H

#include <stddef.h>
#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Platform Backend Interface
 * ========================================================================== */

/**
 * @brief Platform backend function table
 * 
 * Implement this structure for each target platform.
 * All function pointers must be non-NULL for a valid backend.
 */
typedef struct Chip8Platform {
    /** Human-readable name of the platform backend */
    const char* name;
    
    /* === Lifecycle === */
    
    /**
     * @brief Initialize the platform backend
     * 
     * Creates window, initializes audio, etc.
     * 
     * @param ctx CHIP-8 context (platform_data will be set)
     * @param title Window title
     * @param scale Display scale factor (1 = 64x32 pixels)
     * @return true on success, false on failure
     */
    bool (*init)(Chip8Context* ctx, const char* title, int scale);
    
    /**
     * @brief Shutdown the platform backend
     * 
     * Closes window, releases audio resources, etc.
     * 
     * @param ctx CHIP-8 context
     */
    void (*shutdown)(Chip8Context* ctx);
    
    /* === Video === */
    
    /**
     * @brief Render the display buffer to screen
     * 
     * Called after display_dirty is set. Implementation should
     * clear display_dirty after rendering.
     * 
     * @param ctx CHIP-8 context
     */
    void (*render)(Chip8Context* ctx);
    
    /* === Audio === */
    
    /**
     * @brief Start playing the beep sound
     * 
     * Called when sound_timer transitions from 0 to non-zero.
     * 
     * @param ctx CHIP-8 context
     */
    void (*beep_start)(Chip8Context* ctx);
    
    /**
     * @brief Stop playing the beep sound
     * 
     * Called when sound_timer reaches 0.
     * 
     * @param ctx CHIP-8 context
     */
    void (*beep_stop)(Chip8Context* ctx);
    
    /* === Input === */
    
    /**
     * @brief Poll for input events
     * 
     * Updates ctx->keys array and handles quit events.
     * Called once per frame.
     * 
     * @param ctx CHIP-8 context
     */
    void (*poll_events)(Chip8Context* ctx);
    
    /**
     * @brief Poll for menu input events
     * 
     * Returns menu navigation commands.
     * Called when menu is open.
     * 
     * @param ctx CHIP-8 context
     * @return Menu navigation command
     */
    int (*poll_menu_events)(Chip8Context* ctx);
    
    /**
     * @brief Check if quit was requested
     * 
     * @param ctx CHIP-8 context
     * @return true if user requested quit (window close, ESC, etc.)
     */
    bool (*should_quit)(Chip8Context* ctx);
    
    /* === Menu === */
    
    /**
     * @brief Render menu overlay
     * 
     * Draws the pause menu on top of the game display.
     * 
     * @param ctx CHIP-8 context
     * @param menu Menu state to render
     */
    void (*render_menu)(Chip8Context* ctx, void* menu);
    
    /* === Settings === */
    
    /**
     * @brief Apply graphics settings
     * 
     * Updates window scale, fullscreen, colors, etc.
     * 
     * @param ctx CHIP-8 context
     * @param settings Settings to apply
     */
    void (*apply_settings)(Chip8Context* ctx, void* settings);
    
    /* === Timing === */
    
    /**
     * @brief Get current time in microseconds
     * 
     * Used for frame timing and pacing.
     * 
     * @return Current time in microseconds (monotonic)
     */
    uint64_t (*get_time_us)(void);
    
    /**
     * @brief Sleep for specified duration
     * 
     * @param microseconds Duration to sleep
     */
    void (*sleep_us)(uint64_t microseconds);
    
} Chip8Platform;

/* ============================================================================
 * Platform Management
 * ========================================================================== */

/**
 * @brief Register a platform backend
 * 
 * Must be called before chip8_run().
 * 
 * @param platform Platform backend to use
 */
void chip8_set_platform(Chip8Platform* platform);

/**
 * @brief Get the currently registered platform
 * 
 * @return Current platform, or NULL if none registered
 */
Chip8Platform* chip8_get_platform(void);

/* ============================================================================
 * Built-in Platform Backends
 * ========================================================================== */

/**
 * @brief Get the SDL2 platform backend
 * 
 * @return SDL2 platform implementation
 */
Chip8Platform* chip8_platform_sdl2(void);

/**
 * @brief Get a headless platform backend (for testing)
 * 
 * No display or audio, just runs the code.
 * 
 * @return Headless platform implementation
 */
Chip8Platform* chip8_platform_headless(void);

/* ============================================================================
 * Main Loop Interface
 * ========================================================================== */

/**
 * @brief Function signature for recompiled CHIP-8 entry point
 * 
 * The recompiler generates a function with this signature as the
 * main entry point for the recompiled program.
 */
typedef void (*Chip8EntryPoint)(Chip8Context* ctx);

/**
 * @brief Configuration for chip8_run
 */
typedef struct Chip8RunConfig {
    /** Window title */
    const char* title;
    
    /** Display scale factor (default: 10 = 640x320) */
    int scale;
    
    /** Target CPU frequency in Hz (default: 700) */
    int cpu_freq_hz;
    
    /** Enable debug output */
    bool debug;
    
    /** Pointer to embedded ROM data (optional, for sprites/data) */
    const uint8_t* rom_data;
    
    /** Size of embedded ROM data */
    size_t rom_size;
    
    /** Maximum frames to run (0 = unlimited, for headless testing) */
    int max_frames;
    
} Chip8RunConfig;

/**
 * @brief Default run configuration
 */
#define CHIP8_RUN_CONFIG_DEFAULT { \
    .title = "CHIP-8", \
    .scale = 20, \
    .cpu_freq_hz = 700, \
    .debug = false, \
    .rom_data = NULL, \
    .rom_size = 0, \
    .max_frames = 0 \
}

/**
 * @brief Run a recompiled CHIP-8 program
 * 
 * This is the main entry point called from main(). It:
 * 1. Creates and initializes a context
 * 2. Initializes the platform backend
 * 3. Runs the main loop, calling entry_point each frame
 * 4. Cleans up resources on exit
 * 
 * @param entry_point Recompiled program entry function
 * @param config Run configuration
 * @return Exit code (0 = success)
 */
int chip8_run(Chip8EntryPoint entry_point, const Chip8RunConfig* config);

/**
 * @brief Simplified run function with defaults
 * 
 * @param entry_point Recompiled program entry function
 * @param title Window title
 * @return Exit code (0 = success)
 */
int chip8_run_simple(Chip8EntryPoint entry_point, const char* title);

/* ============================================================================
 * Headless Platform (for testing)
 * ========================================================================== */

/**
 * @brief Get the headless platform backend
 * 
 * The headless platform runs without display or audio, suitable for
 * automated testing and CI/CD.
 */
Chip8Platform* chip8_platform_headless(void);

/**
 * @brief Get the current CHIP-8 context
 * 
 * Useful for testing - access the context after chip8_run() returns.
 */
Chip8Context* chip8_get_context(void);

/**
 * @brief Set maximum frames to run in headless mode
 */
void chip8_headless_set_max_frames(Chip8Context* ctx, int max_frames);

/**
 * @brief Dump display to stdout as ASCII art
 */
void chip8_dump_display(Chip8Context* ctx);

/**
 * @brief Calculate a hash of the display buffer
 */
uint32_t chip8_display_hash(Chip8Context* ctx);

/**
 * @brief Dump display to PBM file
 */
bool chip8_dump_display_pbm(Chip8Context* ctx, const char* filename);

/**
 * @brief Compare display against reference PBM file
 */
bool chip8_compare_display_pbm(Chip8Context* ctx, const char* reference_file);

#ifdef __cplusplus
}
#endif

#endif /* CHIP8RT_PLATFORM_H */
