/**
 * @file runtime.c
 * @brief Main runtime implementation
 */

#include "chip8rt/runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

/* ============================================================================
 * Version
 * ========================================================================== */

const char* chip8rt_version(void) {
    return CHIP8RT_VERSION_STRING;
}

/* ============================================================================
 * Platform Management
 * ========================================================================== */

static Chip8Platform* g_platform = NULL;

void chip8_set_platform(Chip8Platform* platform) {
    g_platform = platform;
}

Chip8Platform* chip8_get_platform(void) {
    return g_platform;
}

/* ============================================================================
 * Function Lookup Table (for computed jumps)
 * ========================================================================== */

#define FUNC_TABLE_SIZE 4096

static Chip8FuncPtr g_func_table[FUNC_TABLE_SIZE] = {0};

void chip8_register_function(uint16_t address, Chip8FuncPtr func) {
    if (address < FUNC_TABLE_SIZE) {
        g_func_table[address] = func;
    }
}

Chip8FuncPtr chip8_lookup_function(uint16_t address) {
    if (address < FUNC_TABLE_SIZE) {
        return g_func_table[address];
    }
    return NULL;
}

void chip8_clear_function_table(void) {
    memset(g_func_table, 0, sizeof(g_func_table));
}

/* ============================================================================
 * Debug/Error Handling
 * ========================================================================== */

static bool g_debug_enabled = false;
static Chip8Context* g_context = NULL;

Chip8Context* chip8_get_context(void) {
    return g_context;
}

void chip8_panic(const char* message, uint16_t address) {
    fprintf(stderr, "CHIP-8 PANIC at 0x%03X: %s\n", address, message);
    exit(1);
}

void chip8_debug(const char* format, ...) {
    if (!g_debug_enabled) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[DEBUG] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/* ============================================================================
 * Main Loop
 * ========================================================================== */

int chip8_run(Chip8EntryPoint entry_point, const Chip8RunConfig* config) {
    if (!g_platform) {
        fprintf(stderr, "Error: No platform registered\n");
        return 1;
    }
    
    if (!entry_point) {
        fprintf(stderr, "Error: No entry point provided\n");
        return 1;
    }
    
    /* Use default config if none provided */
    Chip8RunConfig default_cfg = CHIP8_RUN_CONFIG_DEFAULT;
    if (!config) {
        config = &default_cfg;
    }
    
    g_debug_enabled = config->debug;
    
    /* Create context */
    Chip8Context* ctx = chip8_context_create();
    if (!ctx) {
        fprintf(stderr, "Error: Failed to create context\n");
        return 1;
    }
    g_context = ctx;  /* Store global reference for testing */
    
    /* Load ROM data if provided */
    if (config->rom_data && config->rom_size > 0) {
        if (!chip8_context_load_program(ctx, config->rom_data, config->rom_size)) {
            fprintf(stderr, "Error: Failed to load ROM data\n");
            chip8_context_destroy(ctx);
            return 1;
        }
    }
    
    /* Initialize settings - try to load ROM-specific settings first, then global */
    Chip8Settings settings = chip8_settings_default();
    const char* rom_settings_path = chip8_settings_get_rom_path(config->title);
    const char* global_settings_path = chip8_settings_get_default_path();
    const char* settings_path = NULL;  /* Track which path to save to */
    
    if (rom_settings_path && chip8_settings_load(&settings, rom_settings_path)) {
        chip8_debug("Loaded ROM-specific settings from %s", rom_settings_path);
        settings_path = rom_settings_path;
    } else if (global_settings_path && chip8_settings_load(&settings, global_settings_path)) {
        chip8_debug("Loaded global settings from %s", global_settings_path);
        /* Will save to ROM-specific path to not affect other ROMs */
        settings_path = rom_settings_path;
    } else {
        chip8_debug("Using default settings");
        settings_path = rom_settings_path;  /* Save to ROM-specific path */
    }
    
    /* Override with config values if specified (command-line overrides) */
    if (config->scale != 10) {  /* 10 is the default, only override if different */
        settings.graphics.scale = config->scale;
    }
    if (config->cpu_freq_hz != 700) {  /* 700 is the default */
        settings.gameplay.cpu_freq_hz = config->cpu_freq_hz;
    }
    
    /* Initialize menu */
    Chip8MenuState menu;
    chip8_menu_init(&menu, &settings);
    
    /* Seed RNG */
    chip8_random_seed((uint32_t)time(NULL));
    
    /* Initialize platform */
    if (!g_platform->init(ctx, config->title, config->scale)) {
        fprintf(stderr, "Error: Failed to initialize platform\n");
        chip8_context_destroy(ctx);
        return 1;
    }
    
    /* Set max_frames for headless mode if specified */
    if (config->max_frames > 0) {
        chip8_headless_set_max_frames(ctx, config->max_frames);
    }
    
    /* Apply initial settings */
    if (g_platform->apply_settings) {
        g_platform->apply_settings(ctx, &settings);
    }
    
    chip8_debug("Starting main loop (CPU freq: %d Hz)", settings.gameplay.cpu_freq_hz);
    
    /* Timing */
    uint64_t timer_period_us = 1000000 / CHIP8_TIMER_FREQ_HZ;  /* ~16.67ms */
    
    uint64_t last_timer_tick = g_platform->get_time_us();
    
    bool was_beeping = false;
    bool pause_key_released = true;  /* For edge detection on ESC */
    
    /* Save ROM data pointer for reset */
    const uint8_t* rom_data = config->rom_data;
    size_t rom_size = config->rom_size;
    
    /* Main loop */
    while (ctx->running && !g_platform->should_quit(ctx)) {
        uint64_t frame_start = g_platform->get_time_us();
        
        /* Check for pause toggle (ESC or P) */
        if (!chip8_menu_is_open(&menu)) {
            /* Game running - check for pause */
            g_platform->poll_events(ctx);
            
            /* Poll for ESC to open menu */
            int nav = g_platform->poll_menu_events ? g_platform->poll_menu_events(ctx) : 0;
            if (nav == CHIP8_NAV_BACK && pause_key_released) {
                chip8_menu_open(&menu);
                pause_key_released = false;
                continue;
            }
            pause_key_released = (nav != CHIP8_NAV_BACK);
        } else {
            /* Menu open - handle menu input */
            int nav = g_platform->poll_menu_events ? g_platform->poll_menu_events(ctx) : 0;
            
            if (nav != CHIP8_NAV_NONE) {
                chip8_menu_navigate(&menu, nav);
                
                /* Apply settings if changed */
                if (menu.settings_dirty && g_platform->apply_settings) {
                    chip8_menu_apply_settings(&menu, &settings);
                    g_platform->apply_settings(ctx, &settings);
                }
            }
            
            /* Check for quit request */
            if (menu.quit_requested) {
                ctx->running = false;
                break;
            }
            
            /* Check for menu request (multi-ROM launcher) */
            if (menu.menu_requested) {
                ctx->running = false;
                /* Set a flag that rom_selector.cpp can check */
                extern void chip8_request_return_to_menu(void);
                chip8_request_return_to_menu();
                break;
            }
            
            /* Check for reset request */
            if (menu.reset_requested) {
                menu.reset_requested = false;
                chip8_context_reset(ctx);
                if (rom_data && rom_size > 0) {
                    chip8_context_load_program(ctx, rom_data, rom_size);
                }
                chip8_debug("Game reset");
            }
            
            /* Render game (frozen) then menu overlay */
            g_platform->render(ctx);
            if (g_platform->render_menu) {
                g_platform->render_menu(ctx, &menu);
            }
            
            /* Frame pacing */
            uint64_t frame_time = g_platform->get_time_us() - frame_start;
            if (frame_time < timer_period_us) {
                g_platform->sleep_us(timer_period_us - frame_time);
            }
            continue;
        }
        
        /* Handle key wait (FX0A) */
        if (ctx->waiting_for_key) {
            if (ctx->last_key_released >= 0) {
                ctx->V[ctx->key_wait_register] = (uint8_t)ctx->last_key_released;
                ctx->waiting_for_key = false;
                ctx->last_key_released = -1;
            }
        }
        
        /* Execute instructions if not waiting */
        if (!ctx->waiting_for_key) {
            /* Run one "frame" worth of instructions */
            int cycles_per_frame = settings.gameplay.cpu_freq_hz / CHIP8_TIMER_FREQ_HZ;
            ctx->cycles_remaining = cycles_per_frame;
            
            /* Call entry point - it will yield back after cycles_remaining instructions */
            entry_point(ctx);
            ctx->instruction_count += (cycles_per_frame - ctx->cycles_remaining);
        }
        
        /* Timer tick (60Hz) */
        uint64_t now = g_platform->get_time_us();
        if (now - last_timer_tick >= timer_period_us) {
            chip8_tick_timers(ctx);
            last_timer_tick = now;
            ctx->frame_count++;
            
            /* Handle sound */
            bool is_beeping = chip8_sound_active(ctx);
            if (is_beeping && !was_beeping) {
                g_platform->beep_start(ctx);
            } else if (!is_beeping && was_beeping) {
                g_platform->beep_stop(ctx);
            }
            was_beeping = is_beeping;
        }
        
        /* Always render every frame for ImGui overlay responsiveness */
        g_platform->render(ctx);
        ctx->display_dirty = false;
        
        /* Frame pacing - target 60fps */
        uint64_t frame_time = g_platform->get_time_us() - frame_start;
        if (frame_time < timer_period_us) {
            g_platform->sleep_us(timer_period_us - frame_time);
        }
    }
    
    chip8_debug("Shutting down after %llu frames, %llu instructions",
                ctx->frame_count, ctx->instruction_count);
    
    /* Save settings before shutdown */
    if (settings_path) {
        if (chip8_settings_save(&settings, settings_path)) {
            chip8_debug("Saved settings to %s", settings_path);
        } else {
            chip8_debug("Failed to save settings to %s", settings_path);
        }
    }
    
    /* Cleanup */
    g_platform->beep_stop(ctx);
    g_platform->shutdown(ctx);
    chip8_context_destroy(ctx);
    
    return 0;
}

int chip8_run_simple(Chip8EntryPoint entry_point, const char* title) {
    Chip8RunConfig config = CHIP8_RUN_CONFIG_DEFAULT;
    config.title = title;
    return chip8_run(entry_point, &config);
}
