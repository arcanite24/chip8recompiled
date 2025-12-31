/**
 * @file platform_headless.c
 * @brief Headless platform backend for automated testing
 * 
 * This platform runs without any display or audio, suitable for:
 * - CI/CD automated testing
 * - Validating recompiled ROMs against reference outputs
 * - Benchmarking
 */

#include "chip8rt/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Headless Platform Data
 * ========================================================================== */

typedef struct {
    int frames_run;
    int max_frames;
    bool dump_display;
    char* output_file;
} HeadlessPlatformData;

/* ============================================================================
 * Platform Implementation
 * ========================================================================== */

static bool headless_init(Chip8Context* ctx, const char* title, int scale) {
    (void)title;
    (void)scale;
    
    HeadlessPlatformData* data = calloc(1, sizeof(HeadlessPlatformData));
    if (!data) {
        return false;
    }
    
    data->frames_run = 0;
    data->max_frames = 60;  /* Default: 1 second */
    data->dump_display = false;
    data->output_file = NULL;
    
    ctx->platform_data = data;
    return true;
}

static void headless_shutdown(Chip8Context* ctx) {
    HeadlessPlatformData* data = ctx->platform_data;
    if (data) {
        free(data->output_file);
        free(data);
    }
    ctx->platform_data = NULL;
}

static void headless_render(Chip8Context* ctx) {
    ctx->display_dirty = false;
}

static void headless_beep_start(Chip8Context* ctx) {
    (void)ctx;
}

static void headless_beep_stop(Chip8Context* ctx) {
    (void)ctx;
}

static void headless_poll_events(Chip8Context* ctx) {
    HeadlessPlatformData* data = ctx->platform_data;
    
    data->frames_run++;
    
    /* Stop after max_frames */
    if (data->max_frames > 0 && data->frames_run >= data->max_frames) {
        ctx->running = false;
    }
}

static int headless_poll_menu_events(Chip8Context* ctx) {
    (void)ctx;
    return 0;  /* No menu input in headless mode */
}

static bool headless_should_quit(Chip8Context* ctx) {
    HeadlessPlatformData* data = ctx->platform_data;
    return data->max_frames > 0 && data->frames_run >= data->max_frames;
}

static void headless_render_menu(Chip8Context* ctx, void* menu) {
    (void)ctx;
    (void)menu;
}

static void headless_apply_settings(Chip8Context* ctx, void* settings) {
    (void)ctx;
    (void)settings;
}

static uint64_t headless_get_time_us(void) {
    static uint64_t ticks = 0;
    return ticks += 16667;  /* Simulate 60 FPS (16.67ms per frame) */
}

static void headless_sleep_us(uint64_t microseconds) {
    (void)microseconds;
    /* No delay in headless mode - run as fast as possible */
}

/* ============================================================================
 * Platform Backend
 * ========================================================================== */

static Chip8Platform g_headless_platform = {
    .name = "Headless (Testing)",
    .init = headless_init,
    .shutdown = headless_shutdown,
    .render = headless_render,
    .beep_start = headless_beep_start,
    .beep_stop = headless_beep_stop,
    .poll_events = headless_poll_events,
    .poll_menu_events = headless_poll_menu_events,
    .should_quit = headless_should_quit,
    .render_menu = headless_render_menu,
    .apply_settings = headless_apply_settings,
    .get_time_us = headless_get_time_us,
    .sleep_us = headless_sleep_us,
};

Chip8Platform* chip8_platform_headless(void) {
    return &g_headless_platform;
}

/* ============================================================================
 * Test Helpers
 * ========================================================================== */

void chip8_headless_set_max_frames(Chip8Context* ctx, int max_frames) {
    HeadlessPlatformData* data = ctx->platform_data;
    if (data) {
        data->max_frames = max_frames;
    }
}

/**
 * @brief Dump display to stdout as ASCII art
 */
void chip8_dump_display(Chip8Context* ctx) {
    printf("\n");
    for (int y = 0; y < CHIP8_DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++) {
            printf("%c", ctx->display[y * CHIP8_DISPLAY_WIDTH + x] ? '#' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * @brief Dump display as a compact hash for comparison
 * 
 * Creates a simple hash of the display buffer for quick comparison.
 */
uint32_t chip8_display_hash(Chip8Context* ctx) {
    uint32_t hash = 0;
    for (int i = 0; i < CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT; i++) {
        hash = hash * 31 + ctx->display[i];
    }
    return hash;
}

/**
 * @brief Dump display to file in PBM (Portable BitMap) format
 */
bool chip8_dump_display_pbm(Chip8Context* ctx, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        return false;
    }
    
    fprintf(f, "P1\n");
    fprintf(f, "# CHIP-8 Display Dump\n");
    fprintf(f, "%d %d\n", CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT);
    
    for (int y = 0; y < CHIP8_DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++) {
            fprintf(f, "%d ", ctx->display[y * CHIP8_DISPLAY_WIDTH + x] ? 1 : 0);
        }
        fprintf(f, "\n");
    }
    
    fclose(f);
    return true;
}

/**
 * @brief Compare display against a reference PBM file
 * 
 * @return true if display matches reference, false otherwise
 */
bool chip8_compare_display_pbm(Chip8Context* ctx, const char* reference_file) {
    FILE* f = fopen(reference_file, "r");
    if (!f) {
        fprintf(stderr, "Could not open reference file: %s\n", reference_file);
        return false;
    }
    
    char header[16];
    int width, height;
    
    /* Read PBM header */
    if (fscanf(f, "%15s", header) != 1 || strcmp(header, "P1") != 0) {
        fclose(f);
        return false;
    }
    
    /* Skip whitespace and comments */
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '#') {
            /* Skip comment line */
            while ((c = fgetc(f)) != '\n' && c != EOF);
        } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            /* Non-whitespace, non-comment - put it back */
            ungetc(c, f);
            break;
        }
    }
    
    if (fscanf(f, "%d %d", &width, &height) != 2) {
        fclose(f);
        return false;
    }
    
    if (width != CHIP8_DISPLAY_WIDTH || height != CHIP8_DISPLAY_HEIGHT) {
        fclose(f);
        return false;
    }
    
    /* Compare pixels */
    bool match = true;
    for (int i = 0; i < width * height && match; i++) {
        int pixel;
        if (fscanf(f, "%d", &pixel) != 1) {
            match = false;
            break;
        }
        if ((pixel != 0) != (ctx->display[i] != 0)) {
            match = false;
        }
    }
    
    fclose(f);
    return match;
}
