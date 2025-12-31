/**
 * @file context.c
 * @brief CHIP-8 context implementation
 */

#include "chip8rt/context.h"
#include <stdlib.h>
#include <string.h>

/* Built-in 4x5 font sprites (0-F) */
static const uint8_t CHIP8_FONT[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
    0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
    0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
    0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
    0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
    0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
    0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
    0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
    0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
    0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
    0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
    0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
    0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
    0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
    0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
    0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

Chip8Context* chip8_context_create(void) {
    Chip8Context* ctx = (Chip8Context*)calloc(1, sizeof(Chip8Context));
    if (!ctx) {
        return NULL;
    }
    
    /* Load font into memory */
    memcpy(&ctx->memory[CHIP8_FONT_START], CHIP8_FONT, sizeof(CHIP8_FONT));
    
    /* Set initial state */
    ctx->PC = CHIP8_PROGRAM_START;
    ctx->running = true;
    ctx->last_key_released = -1;
    
    return ctx;
}

void chip8_context_destroy(Chip8Context* ctx) {
    if (ctx) {
        free(ctx);
    }
}

void chip8_context_reset(Chip8Context* ctx) {
    if (!ctx) return;
    
    /* Save font (it's constant anyway) */
    
    /* Clear registers */
    memset(ctx->V, 0, sizeof(ctx->V));
    ctx->I = 0;
    ctx->PC = CHIP8_PROGRAM_START;
    ctx->SP = 0;
    
    /* Clear timers */
    ctx->delay_timer = 0;
    ctx->sound_timer = 0;
    
    /* Clear stack */
    memset(ctx->stack, 0, sizeof(ctx->stack));
    
    /* Clear display */
    memset(ctx->display, 0, sizeof(ctx->display));
    ctx->display_dirty = true;
    
    /* Clear input */
    memset(ctx->keys, 0, sizeof(ctx->keys));
    ctx->last_key_released = -1;
    
    /* Reset runtime state */
    ctx->running = true;
    ctx->waiting_for_key = false;
    ctx->key_wait_register = 0;
    
    /* Reset stats */
    ctx->instruction_count = 0;
    ctx->frame_count = 0;
}

bool chip8_context_load_program(Chip8Context* ctx, 
                                 const uint8_t* program_data, 
                                 size_t size) {
    if (!ctx || !program_data) {
        return false;
    }
    
    /* Check if program fits */
    if (size > (CHIP8_MEMORY_SIZE - CHIP8_PROGRAM_START)) {
        return false;
    }
    
    /* Copy program into memory */
    memcpy(&ctx->memory[CHIP8_PROGRAM_START], program_data, size);
    
    return true;
}
