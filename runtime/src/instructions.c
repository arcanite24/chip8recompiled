/**
 * @file instructions.c
 * @brief CHIP-8 instruction helper implementations
 */

#include "chip8rt/instructions.h"
#include <stdlib.h>
#include <string.h>

/* Random number generator state */
static uint32_t rng_state = 0x12345678;

void chip8_clear_screen(Chip8Context* ctx) {
    memset(ctx->display, 0, sizeof(ctx->display));
    ctx->display_dirty = true;
}

void chip8_draw_sprite(Chip8Context* ctx, uint8_t vx, uint8_t vy, uint8_t height) {
    /* Get coordinates (wrap around screen) */
    uint8_t x = ctx->V[vx] % CHIP8_DISPLAY_WIDTH;
    uint8_t y = ctx->V[vy] % CHIP8_DISPLAY_HEIGHT;
    
    /* Reset collision flag */
    ctx->V[0xF] = 0;
    
    /* Draw each row of the sprite */
    for (uint8_t row = 0; row < height; ++row) {
        uint8_t sprite_byte = ctx->memory[ctx->I + row];
        
        /* Don't draw past bottom of screen */
        if (y + row >= CHIP8_DISPLAY_HEIGHT) {
            break;
        }
        
        /* Draw 8 pixels of this row */
        for (uint8_t col = 0; col < 8; ++col) {
            /* Don't draw past right edge of screen */
            if (x + col >= CHIP8_DISPLAY_WIDTH) {
                break;
            }
            
            /* Check if sprite pixel is set */
            if (sprite_byte & (0x80 >> col)) {
                size_t pixel_idx = (y + row) * CHIP8_DISPLAY_WIDTH + (x + col);
                
                /* Check for collision (pixel already on) */
                if (ctx->display[pixel_idx]) {
                    ctx->V[0xF] = 1;
                }
                
                /* XOR the pixel */
                ctx->display[pixel_idx] ^= 1;
            }
        }
    }
    
    ctx->display_dirty = true;
}

bool chip8_key_pressed(Chip8Context* ctx, uint8_t key) {
    if (key > 0xF) {
        return false;
    }
    return ctx->keys[key];
}

void chip8_wait_key(Chip8Context* ctx, uint8_t reg) {
    /* 
     * This blocks until a key is pressed and released.
     * The main loop should check waiting_for_key and handle input.
     */
    ctx->waiting_for_key = true;
    ctx->key_wait_register = reg;
    
    /* 
     * In a real implementation, we'd return here and let the main loop
     * handle the waiting. The key value gets stored when a key is released.
     */
}

void chip8_store_bcd(Chip8Context* ctx, uint8_t x) {
    uint8_t value = ctx->V[x];
    
    /* Store hundreds digit */
    ctx->memory[ctx->I] = value / 100;
    
    /* Store tens digit */
    ctx->memory[ctx->I + 1] = (value / 10) % 10;
    
    /* Store ones digit */
    ctx->memory[ctx->I + 2] = value % 10;
}

void chip8_store_registers(Chip8Context* ctx, uint8_t x, bool increment_i) {
    for (uint8_t i = 0; i <= x; ++i) {
        ctx->memory[ctx->I + i] = ctx->V[i];
    }
    
    if (increment_i) {
        ctx->I += x + 1;
    }
}

void chip8_load_registers(Chip8Context* ctx, uint8_t x, bool increment_i) {
    for (uint8_t i = 0; i <= x; ++i) {
        ctx->V[i] = ctx->memory[ctx->I + i];
    }
    
    if (increment_i) {
        ctx->I += x + 1;
    }
}

uint8_t chip8_random_byte(void) {
    /* Simple xorshift RNG */
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (uint8_t)(rng_state & 0xFF);
}

void chip8_random_seed(uint32_t seed) {
    rng_state = seed;
    if (rng_state == 0) {
        rng_state = 0x12345678;
    }
}

void chip8_tick_timers(Chip8Context* ctx) {
    if (ctx->delay_timer > 0) {
        ctx->delay_timer--;
    }
    
    if (ctx->sound_timer > 0) {
        ctx->sound_timer--;
    }
}
