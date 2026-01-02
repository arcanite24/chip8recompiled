/**
 * @file platform_sdl.c
 * @brief SDL2 platform backend implementation
 */

#include "chip8rt/platform.h"
#include "chip8rt/settings.h"
#include "chip8rt/menu.h"
#include "chip8rt/imgui_overlay.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * Gamepad State
 * ========================================================================== */

typedef struct {
    SDL_GameController* controller;
    SDL_JoystickID joystick_id;
    char name[128];
    bool connected;
    bool has_rumble;
    int player_index;
} GamepadState;

/* ============================================================================
 * Platform Data
 * ========================================================================== */

typedef struct {
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;
    SDL_AudioDeviceID audio_device;
    int scale;
    bool quit_requested;
    bool escape_pressed;  /* Track ESC press for menu toggle */
    
    /* Audio state */
    float audio_phase;
    bool audio_playing;
    float audio_volume;
    int audio_frequency;
    Chip8Waveform audio_waveform;
    
    /* Color theme */
    Chip8Color fg_color;
    Chip8Color bg_color;
    
    /* Visual effects */
    bool pixel_grid;
    bool crt_effect;
    float scanline_intensity;
    
    /* Key repeat rate limiting */
    uint64_t key_repeat_time[16];  /* Last time each key triggered a repeat */
    bool key_first_press[16];      /* Is this the first press? */
    uint32_t key_repeat_delay_us;
    uint32_t key_repeat_rate_us;
    
    /* Menu navigation repeat */
    uint64_t menu_repeat_time;
    int last_menu_nav;
    
    /* ImGui overlay */
    Chip8OverlayState overlay_state;
    bool overlay_enabled;
    Chip8Settings* settings_ref;  /* Reference to settings for overlay */
    
    /* Gamepad support */
    GamepadState gamepads[CHIP8_MAX_GAMEPADS];
    int active_gamepad_idx;
    int gamepad_count;
    bool gamepad_enabled;
    float analog_deadzone;
    bool use_left_stick;
    bool use_dpad;
    bool vibration_enabled;
    float vibration_intensity;
    
    /* Input remapping state */
    bool waiting_for_remap;      /* Currently waiting for key input */
    int remap_target_key;        /* Which CHIP-8 key we're remapping */
    bool remap_is_gamepad;       /* Are we remapping gamepad or keyboard? */
    bool remap_is_alternate;     /* Are we setting the alternate key? */
    
    /* Configurable key bindings (copied from settings) */
    Chip8KeyBinding key_bindings[16];
} SDLPlatformData;

/* Key repeat default settings (in microseconds) */
#define KEY_REPEAT_DELAY_US  200000  /* 200ms before repeat starts */
#define KEY_REPEAT_RATE_US   100000  /* 100ms between repeats */

/* Menu navigation repeat (in microseconds) */
#define MENU_REPEAT_DELAY_US  300000  /* 300ms before repeat */
#define MENU_REPEAT_RATE_US   150000  /* 150ms between repeats */

/* Forward declarations */
static uint64_t sdl_get_time_us(void);
static void sdl_apply_settings(Chip8Context* ctx, void* settings_ptr);

/* ============================================================================
 * Key Mapping
 * ========================================================================== */

/*
 * CHIP-8 Keypad:     Keyboard Mapping:
 *  1 2 3 C            1 2 3 4
 *  4 5 6 D            Q W E R
 *  7 8 9 E            A S D F
 *  A 0 B F            Z X C V
 */
static const SDL_Scancode KEY_MAP[16] = {
    SDL_SCANCODE_X,    /* 0 */
    SDL_SCANCODE_1,    /* 1 */
    SDL_SCANCODE_2,    /* 2 */
    SDL_SCANCODE_3,    /* 3 */
    SDL_SCANCODE_Q,    /* 4 */
    SDL_SCANCODE_W,    /* 5 */
    SDL_SCANCODE_E,    /* 6 */
    SDL_SCANCODE_A,    /* 7 */
    SDL_SCANCODE_S,    /* 8 */
    SDL_SCANCODE_D,    /* 9 */
    SDL_SCANCODE_Z,    /* A */
    SDL_SCANCODE_C,    /* B */
    SDL_SCANCODE_4,    /* C */
    SDL_SCANCODE_R,    /* D */
    SDL_SCANCODE_F,    /* E */
    SDL_SCANCODE_V     /* F */
};

/* ============================================================================
 * Gamepad Helper Functions
 * ========================================================================== */

/**
 * @brief Convert SDL_GameControllerButton to Chip8GamepadButton
 */
static Chip8GamepadButton sdl_button_to_chip8(SDL_GameControllerButton button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return CHIP8_GPAD_A;
        case SDL_CONTROLLER_BUTTON_B: return CHIP8_GPAD_B;
        case SDL_CONTROLLER_BUTTON_X: return CHIP8_GPAD_X;
        case SDL_CONTROLLER_BUTTON_Y: return CHIP8_GPAD_Y;
        case SDL_CONTROLLER_BUTTON_BACK: return CHIP8_GPAD_BACK;
        case SDL_CONTROLLER_BUTTON_GUIDE: return CHIP8_GPAD_GUIDE;
        case SDL_CONTROLLER_BUTTON_START: return CHIP8_GPAD_START;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return CHIP8_GPAD_LEFTSTICK;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return CHIP8_GPAD_RIGHTSTICK;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return CHIP8_GPAD_LEFTSHOULDER;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return CHIP8_GPAD_RIGHTSHOULDER;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return CHIP8_GPAD_DPAD_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return CHIP8_GPAD_DPAD_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return CHIP8_GPAD_DPAD_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return CHIP8_GPAD_DPAD_RIGHT;
        default: return CHIP8_GPAD_NONE;
    }
}

/**
 * @brief Convert Chip8GamepadButton to SDL_GameControllerButton
 */
static SDL_GameControllerButton chip8_button_to_sdl(Chip8GamepadButton button) {
    switch (button) {
        case CHIP8_GPAD_A: return SDL_CONTROLLER_BUTTON_A;
        case CHIP8_GPAD_B: return SDL_CONTROLLER_BUTTON_B;
        case CHIP8_GPAD_X: return SDL_CONTROLLER_BUTTON_X;
        case CHIP8_GPAD_Y: return SDL_CONTROLLER_BUTTON_Y;
        case CHIP8_GPAD_BACK: return SDL_CONTROLLER_BUTTON_BACK;
        case CHIP8_GPAD_GUIDE: return SDL_CONTROLLER_BUTTON_GUIDE;
        case CHIP8_GPAD_START: return SDL_CONTROLLER_BUTTON_START;
        case CHIP8_GPAD_LEFTSTICK: return SDL_CONTROLLER_BUTTON_LEFTSTICK;
        case CHIP8_GPAD_RIGHTSTICK: return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
        case CHIP8_GPAD_LEFTSHOULDER: return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        case CHIP8_GPAD_RIGHTSHOULDER: return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        case CHIP8_GPAD_DPAD_UP: return SDL_CONTROLLER_BUTTON_DPAD_UP;
        case CHIP8_GPAD_DPAD_DOWN: return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        case CHIP8_GPAD_DPAD_LEFT: return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        case CHIP8_GPAD_DPAD_RIGHT: return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        default: return SDL_CONTROLLER_BUTTON_INVALID;
    }
}

/**
 * @brief Initialize gamepad subsystem and detect connected controllers
 */
static void init_gamepads(SDLPlatformData* data) {
    /* Initialize game controller subsystem */
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "Warning: Failed to init game controller subsystem: %s\n", SDL_GetError());
        return;
    }
    
    /* Enable controller events */
    SDL_GameControllerEventState(SDL_ENABLE);
    
    /* Default settings */
    data->gamepad_enabled = true;
    data->analog_deadzone = 0.25f;
    data->use_left_stick = true;
    data->use_dpad = true;
    data->vibration_enabled = true;
    data->vibration_intensity = 0.5f;
    data->active_gamepad_idx = 0;
    data->gamepad_count = 0;
    
    /* Initialize gamepad slots */
    for (int i = 0; i < CHIP8_MAX_GAMEPADS; i++) {
        data->gamepads[i].controller = NULL;
        data->gamepads[i].connected = false;
        data->gamepads[i].name[0] = '\0';
    }
    
    /* Detect already connected controllers */
    int num_joysticks = SDL_NumJoysticks();
    printf("[Gamepad] Detected %d joystick(s)\n", num_joysticks);
    
    for (int i = 0; i < num_joysticks && data->gamepad_count < CHIP8_MAX_GAMEPADS; i++) {
        if (SDL_IsGameController(i)) {
            SDL_GameController* controller = SDL_GameControllerOpen(i);
            if (controller) {
                int slot = data->gamepad_count;
                data->gamepads[slot].controller = controller;
                data->gamepads[slot].joystick_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
                data->gamepads[slot].connected = true;
                data->gamepads[slot].player_index = slot;
                data->gamepads[slot].has_rumble = SDL_GameControllerHasRumble(controller);
                
                const char* name = SDL_GameControllerName(controller);
                strncpy(data->gamepads[slot].name, name ? name : "Unknown Controller", 
                        sizeof(data->gamepads[slot].name) - 1);
                
                printf("[Gamepad] Connected: %s (slot %d, rumble: %s)\n", 
                       data->gamepads[slot].name, slot,
                       data->gamepads[slot].has_rumble ? "yes" : "no");
                
                data->gamepad_count++;
            }
        }
    }
    
    if (data->gamepad_count > 0) {
        printf("[Gamepad] %d controller(s) ready\n", data->gamepad_count);
    }
}

/**
 * @brief Handle gamepad hotplug events
 */
static void handle_gamepad_added(SDLPlatformData* data, int device_index) {
    if (!SDL_IsGameController(device_index)) return;
    
    /* Find empty slot */
    int slot = -1;
    for (int i = 0; i < CHIP8_MAX_GAMEPADS; i++) {
        if (!data->gamepads[i].connected) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        printf("[Gamepad] No free slots for new controller\n");
        return;
    }
    
    SDL_GameController* controller = SDL_GameControllerOpen(device_index);
    if (controller) {
        data->gamepads[slot].controller = controller;
        data->gamepads[slot].joystick_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
        data->gamepads[slot].connected = true;
        data->gamepads[slot].player_index = slot;
        data->gamepads[slot].has_rumble = SDL_GameControllerHasRumble(controller);
        
        const char* name = SDL_GameControllerName(controller);
        strncpy(data->gamepads[slot].name, name ? name : "Unknown Controller",
                sizeof(data->gamepads[slot].name) - 1);
        
        data->gamepad_count++;
        
        printf("[Gamepad] Added: %s (slot %d)\n", data->gamepads[slot].name, slot);
    }
}

/**
 * @brief Handle gamepad removal events
 */
static void handle_gamepad_removed(SDLPlatformData* data, SDL_JoystickID joystick_id) {
    for (int i = 0; i < CHIP8_MAX_GAMEPADS; i++) {
        if (data->gamepads[i].connected && data->gamepads[i].joystick_id == joystick_id) {
            printf("[Gamepad] Removed: %s (slot %d)\n", data->gamepads[i].name, i);
            
            SDL_GameControllerClose(data->gamepads[i].controller);
            data->gamepads[i].controller = NULL;
            data->gamepads[i].connected = false;
            data->gamepads[i].name[0] = '\0';
            data->gamepad_count--;
            
            /* If active gamepad was removed, switch to first available */
            if (data->active_gamepad_idx == i) {
                data->active_gamepad_idx = 0;
                for (int j = 0; j < CHIP8_MAX_GAMEPADS; j++) {
                    if (data->gamepads[j].connected) {
                        data->active_gamepad_idx = j;
                        break;
                    }
                }
            }
            break;
        }
    }
}

/**
 * @brief Trigger haptic feedback on the active gamepad
 */
static void gamepad_rumble(SDLPlatformData* data, float intensity, uint32_t duration_ms) {
    if (!data->vibration_enabled || data->active_gamepad_idx < 0) return;
    
    GamepadState* gpad = &data->gamepads[data->active_gamepad_idx];
    if (!gpad->connected || !gpad->has_rumble || !gpad->controller) return;
    
    uint16_t rumble_strength = (uint16_t)(intensity * data->vibration_intensity * 65535);
    SDL_GameControllerRumble(gpad->controller, rumble_strength, rumble_strength, duration_ms);
}

/**
 * @brief Check if a gamepad button is pressed
 */
static bool is_gamepad_button_pressed(SDLPlatformData* data, Chip8GamepadButton button) {
    if (!data->gamepad_enabled || button == CHIP8_GPAD_NONE) return false;
    
    GamepadState* gpad = &data->gamepads[data->active_gamepad_idx];
    if (!gpad->connected || !gpad->controller) return false;
    
    SDL_GameControllerButton sdl_button = chip8_button_to_sdl(button);
    if (sdl_button == SDL_CONTROLLER_BUTTON_INVALID) return false;
    
    return SDL_GameControllerGetButton(gpad->controller, sdl_button) != 0;
}

/**
 * @brief Get analog stick direction as D-pad equivalent
 */
static void get_analog_stick_direction(SDLPlatformData* data, bool* up, bool* down, bool* left, bool* right) {
    *up = *down = *left = *right = false;
    
    if (!data->gamepad_enabled || !data->use_left_stick) return;
    
    GamepadState* gpad = &data->gamepads[data->active_gamepad_idx];
    if (!gpad->connected || !gpad->controller) return;
    
    int16_t x = SDL_GameControllerGetAxis(gpad->controller, SDL_CONTROLLER_AXIS_LEFTX);
    int16_t y = SDL_GameControllerGetAxis(gpad->controller, SDL_CONTROLLER_AXIS_LEFTY);
    
    float deadzone = data->analog_deadzone * 32767;
    
    if (x < -deadzone) *left = true;
    if (x > deadzone) *right = true;
    if (y < -deadzone) *up = true;
    if (y > deadzone) *down = true;
}

/**
 * @brief Shutdown gamepad subsystem
 */
static void shutdown_gamepads(SDLPlatformData* data) {
    for (int i = 0; i < CHIP8_MAX_GAMEPADS; i++) {
        if (data->gamepads[i].controller) {
            SDL_GameControllerClose(data->gamepads[i].controller);
            data->gamepads[i].controller = NULL;
        }
    }
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

/* ============================================================================
 * Audio Callback
 * ========================================================================== */

static void audio_callback(void* userdata, uint8_t* stream, int len) {
    SDLPlatformData* data = (SDLPlatformData*)userdata;
    float* buffer = (float*)stream;
    int samples = len / sizeof(float);
    
    const float frequency = (float)data->audio_frequency;
    const float amplitude = data->audio_volume;
    const float sample_rate = 44100.0f;
    
    for (int i = 0; i < samples; ++i) {
        if (data->audio_playing && amplitude > 0.0f) {
            float value = 0.0f;
            float phase = data->audio_phase;
            
            switch (data->audio_waveform) {
                case CHIP8_WAVE_SQUARE:
                    value = (sinf(phase * 2.0f * 3.14159f) > 0) ? amplitude : -amplitude;
                    break;
                case CHIP8_WAVE_SINE:
                    value = sinf(phase * 2.0f * 3.14159f) * amplitude;
                    break;
                case CHIP8_WAVE_TRIANGLE:
                    value = (2.0f * fabsf(2.0f * (phase - floorf(phase + 0.5f))) - 1.0f) * amplitude;
                    break;
                case CHIP8_WAVE_SAWTOOTH:
                    value = (2.0f * (phase - floorf(phase + 0.5f))) * amplitude;
                    break;
                case CHIP8_WAVE_NOISE:
                    value = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * amplitude * 0.5f;
                    break;
                default:
                    value = (sinf(phase * 2.0f * 3.14159f) > 0) ? amplitude : -amplitude;
                    break;
            }
            
            buffer[i] = value;
            
            data->audio_phase += frequency / sample_rate;
            if (data->audio_phase >= 1.0f) {
                data->audio_phase -= 1.0f;
            }
        } else {
            buffer[i] = 0;
        }
    }
}

/* ============================================================================
 * Platform Implementation
 * ========================================================================== */

static bool sdl_init(Chip8Context* ctx, const char* title, int scale) {
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    
    /* Allocate platform data */
    SDLPlatformData* data = (SDLPlatformData*)calloc(1, sizeof(SDLPlatformData));
    if (!data) {
        SDL_Quit();
        return false;
    }
    
    data->scale = scale;
    ctx->platform_data = data;
    
    /* Initialize default settings */
    data->audio_volume = 0.3f;
    data->audio_frequency = 440;
    data->audio_waveform = CHIP8_WAVE_SQUARE;
    data->fg_color = (Chip8Color){255, 255, 255, 255};
    data->bg_color = (Chip8Color){0, 0, 0, 255};
    data->key_repeat_delay_us = KEY_REPEAT_DELAY_US;
    data->key_repeat_rate_us = KEY_REPEAT_RATE_US;
    data->menu_repeat_time = 0;
    data->last_menu_nav = 0;
    
    /* Initialize key repeat state */
    for (int i = 0; i < 16; ++i) {
        data->key_first_press[i] = true;
        data->key_repeat_time[i] = 0;
    }
    
    /* Create window */
    int width = CHIP8_DISPLAY_WIDTH * scale;
    int height = CHIP8_DISPLAY_HEIGHT * scale;
    
    data->window = SDL_CreateWindow(
        title ? title : "CHIP-8",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );
    
    if (!data->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        free(data);
        SDL_Quit();
        return false;
    }
    
    /* Create renderer */
    data->renderer = SDL_CreateRenderer(
        data->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!data->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(data->window);
        free(data);
        SDL_Quit();
        return false;
    }
    
    /* Create texture for display */
    data->texture = SDL_CreateTexture(
        data->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        CHIP8_DISPLAY_WIDTH,
        CHIP8_DISPLAY_HEIGHT
    );
    
    if (!data->texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(data->renderer);
        SDL_DestroyWindow(data->window);
        free(data);
        SDL_Quit();
        return false;
    }
    
    /* Initialize audio */
    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_callback;
    want.userdata = data;
    
    data->audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (data->audio_device == 0) {
        fprintf(stderr, "Warning: SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        /* Continue without audio */
    } else {
        SDL_PauseAudioDevice(data->audio_device, 0);  /* Start audio */
    }
    
    /* Initialize ImGui overlay */
    if (!chip8_overlay_init(data->window, data->renderer)) {
        fprintf(stderr, "Warning: Failed to initialize ImGui overlay\n");
        /* Continue without overlay */
    }
    
    /* Initialize overlay state */
    memset(&data->overlay_state, 0, sizeof(data->overlay_state));
    data->overlay_state.show_fps = true;  /* Show FPS by default */
    data->overlay_enabled = true;
    
    /* Initialize gamepad support */
    init_gamepads(data);
    
    /* Initialize default key bindings */
    Chip8InputSettings default_input;
    chip8_input_settings_default(&default_input);
    memcpy(data->key_bindings, default_input.bindings, sizeof(data->key_bindings));
    
    /* Set initial display to black */
    SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, 255);
    SDL_RenderClear(data->renderer);
    SDL_RenderPresent(data->renderer);
    
    return true;
}

static void sdl_shutdown(Chip8Context* ctx) {
    SDLPlatformData* data = (SDLPlatformData*)ctx->platform_data;
    if (!data) return;
    
    /* Shutdown gamepads */
    shutdown_gamepads(data);
    
    /* Shutdown ImGui */
    chip8_overlay_shutdown();
    
    if (data->audio_device) {
        SDL_CloseAudioDevice(data->audio_device);
    }
    if (data->texture) {
        SDL_DestroyTexture(data->texture);
    }
    if (data->renderer) {
        SDL_DestroyRenderer(data->renderer);
    }
    if (data->window) {
        SDL_DestroyWindow(data->window);
    }
    
    free(data);
    ctx->platform_data = NULL;
    
    SDL_Quit();
}

static void sdl_render(Chip8Context* ctx) {
    SDLPlatformData* data = (SDLPlatformData*)ctx->platform_data;
    if (!data) return;
    
    /* Update texture with display contents using current colors */
    uint32_t pixels[CHIP8_DISPLAY_SIZE];
    
    /* Convert colors to RGBA8888 format */
    uint32_t fg = ((uint32_t)data->fg_color.r << 24) | 
                  ((uint32_t)data->fg_color.g << 16) | 
                  ((uint32_t)data->fg_color.b << 8) | 
                  (uint32_t)data->fg_color.a;
    uint32_t bg = ((uint32_t)data->bg_color.r << 24) | 
                  ((uint32_t)data->bg_color.g << 16) | 
                  ((uint32_t)data->bg_color.b << 8) | 
                  (uint32_t)data->bg_color.a;
    
    for (int i = 0; i < CHIP8_DISPLAY_SIZE; ++i) {
        pixels[i] = ctx->display[i] ? fg : bg;
    }
    
    SDL_UpdateTexture(data->texture, NULL, pixels, CHIP8_DISPLAY_WIDTH * sizeof(uint32_t));
    
    /* Clear and draw */
    SDL_RenderClear(data->renderer);
    SDL_RenderCopy(data->renderer, data->texture, NULL, NULL);
    
    /* Draw pixel grid overlay if enabled */
    if (data->pixel_grid && data->scale >= 2) {
        SDL_SetRenderDrawBlendMode(data->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(data->renderer, 40, 40, 40, 100);
        
        int window_width = CHIP8_DISPLAY_WIDTH * data->scale;
        int window_height = CHIP8_DISPLAY_HEIGHT * data->scale;
        
        /* Draw vertical lines */
        for (int x = 0; x <= CHIP8_DISPLAY_WIDTH; ++x) {
            SDL_RenderDrawLine(data->renderer, x * data->scale, 0, 
                               x * data->scale, window_height);
        }
        /* Draw horizontal lines */
        for (int y = 0; y <= CHIP8_DISPLAY_HEIGHT; ++y) {
            SDL_RenderDrawLine(data->renderer, 0, y * data->scale, 
                               window_width, y * data->scale);
        }
    }
    
    /* Draw CRT scanline effect if enabled */
    if (data->crt_effect && data->scanline_intensity > 0.0f) {
        SDL_SetRenderDrawBlendMode(data->renderer, SDL_BLENDMODE_BLEND);
        uint8_t alpha = (uint8_t)(data->scanline_intensity * 128);
        SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, alpha);
        
        int window_height = CHIP8_DISPLAY_HEIGHT * data->scale;
        int window_width = CHIP8_DISPLAY_WIDTH * data->scale;
        
        /* Draw scanlines on every other row */
        for (int y = 0; y < window_height; y += 2) {
            SDL_RenderDrawLine(data->renderer, 0, y, window_width, y);
        }
    }
    
    /* Render ImGui overlay */
    if (data->overlay_enabled) {
        uint64_t current_time = SDL_GetPerformanceCounter() * 1000000 / SDL_GetPerformanceFrequency();
        chip8_overlay_update_fps(&data->overlay_state, current_time);
        
        chip8_overlay_new_frame();
        chip8_overlay_render(ctx, &data->overlay_state, data->settings_ref);
        chip8_overlay_present(data->renderer);
        
        /* Check if ImGui changed settings and apply them */
        if (data->overlay_state.settings_changed && data->settings_ref) {
            data->overlay_state.settings_changed = false;
            /* Apply settings directly here since we have access to the data */
            sdl_apply_settings(ctx, data->settings_ref);
        }
    }
    
    SDL_RenderPresent(data->renderer);
}

static void sdl_beep_start(Chip8Context* ctx) {
    SDLPlatformData* data = (SDLPlatformData*)ctx->platform_data;
    if (data) {
        data->audio_playing = true;
    }
}

static void sdl_beep_stop(Chip8Context* ctx) {
    SDLPlatformData* data = (SDLPlatformData*)ctx->platform_data;
    if (data) {
        data->audio_playing = false;
    }
}

static void sdl_poll_events(Chip8Context* ctx) {
    SDLPlatformData* data = (SDLPlatformData*)ctx->platform_data;
    if (!data) return;
    
    const uint8_t* keyboard = SDL_GetKeyboardState(NULL);
    uint64_t now = sdl_get_time_us();
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        /* Let ImGui process the event first */
        if (data->overlay_enabled) {
            chip8_overlay_process_event(&event);
        }
        
        switch (event.type) {
            case SDL_QUIT:
                data->quit_requested = true;
                ctx->running = false;
                break;
            
            /* Handle gamepad hotplug */
            case SDL_CONTROLLERDEVICEADDED:
                handle_gamepad_added(data, event.cdevice.which);
                break;
            
            case SDL_CONTROLLERDEVICEREMOVED:
                handle_gamepad_removed(data, event.cdevice.which);
                break;
            
            case SDL_KEYDOWN:
                if (event.key.repeat) break;  /* Ignore key repeats for special keys */
                
                /* Handle remapping mode - check overlay state */
                if (data->overlay_state.waiting_for_input && !data->overlay_state.remap_is_gamepad) {
                    /* Capture this key for remapping */
                    int target = data->overlay_state.remap_target_key;
                    if (target >= 0 && target < 16) {
                        if (data->overlay_state.remap_is_alternate) {
                            data->key_bindings[target].keyboard_alt = event.key.keysym.scancode;
                        } else {
                            data->key_bindings[target].keyboard = event.key.keysym.scancode;
                        }
                        /* Update settings if available */
                        if (data->settings_ref) {
                            data->settings_ref->input.bindings[target] = data->key_bindings[target];
                            data->overlay_state.settings_changed = true;
                        }
                    }
                    data->overlay_state.waiting_for_input = false;
                    break;
                }
                
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_ESCAPE:
                        if (data->overlay_state.waiting_for_input) {
                            /* Cancel remapping */
                            data->overlay_state.waiting_for_input = false;
                        } else {
                            /* Toggle ImGui settings (replaces old menu) */
                            data->overlay_state.show_settings = !data->overlay_state.show_settings;
                        }
                        break;
                    case SDL_SCANCODE_F1:
                        /* Toggle FPS display */
                        chip8_overlay_toggle_fps(&data->overlay_state);
                        break;
                    case SDL_SCANCODE_F2:
                        /* Toggle debug overlay */
                        chip8_overlay_toggle_debug(&data->overlay_state);
                        break;
                    case SDL_SCANCODE_F3:
                        /* Toggle settings window */
                        data->overlay_state.show_settings = !data->overlay_state.show_settings;
                        break;
                    case SDL_SCANCODE_F10:
                        /* Toggle entire overlay */
                        data->overlay_enabled = !data->overlay_enabled;
                        break;
                    default:
                        break;
                }
                break;
            
            case SDL_CONTROLLERBUTTONDOWN:
                /* Handle gamepad remapping mode */
                if (data->overlay_state.waiting_for_input && data->overlay_state.remap_is_gamepad) {
                    int target = data->overlay_state.remap_target_key;
                    if (target >= 0 && target < 16) {
                        Chip8GamepadButton btn = sdl_button_to_chip8((SDL_GameControllerButton)event.cbutton.button);
                        data->key_bindings[target].gamepad_button = btn;
                        if (data->settings_ref) {
                            data->settings_ref->input.bindings[target] = data->key_bindings[target];
                            data->overlay_state.settings_changed = true;
                        }
                    }
                    data->overlay_state.waiting_for_input = false;
                }
                break;
                
            case SDL_KEYUP:
                /* Handle key release for FX0A - check configured bindings */
                for (int key = 0; key < 16; ++key) {
                    SDL_Scancode scancode = event.key.keysym.scancode;
                    Chip8KeyBinding* binding = &data->key_bindings[key];
                    
                    if ((int)scancode == binding->keyboard || 
                        (int)scancode == binding->keyboard_alt) {
                        if (ctx->waiting_for_key) {
                            ctx->last_key_released = key;
                            /* Trigger rumble feedback */
                            gamepad_rumble(data, 0.3f, 50);
                        }
                        /* Reset repeat state on key release */
                        data->key_first_press[key] = true;
                        data->key_repeat_time[key] = 0;
                    }
                }
                break;
        }
    }
    
    /* Save previous key state for edge detection */
    for (int key = 0; key < 16; ++key) {
        ctx->keys_prev[key] = ctx->keys[key];
    }
    
    /* Get analog stick directions for directional keys */
    bool stick_up, stick_down, stick_left, stick_right;
    get_analog_stick_direction(data, &stick_up, &stick_down, &stick_left, &stick_right);
    
    /* Update key states with repeat rate limiting */
    for (int key = 0; key < 16; ++key) {
        Chip8KeyBinding* binding = &data->key_bindings[key];
        
        /* Check keyboard bindings */
        bool physical_pressed = false;
        if (binding->keyboard >= 0 && keyboard[binding->keyboard]) {
            physical_pressed = true;
        }
        if (!physical_pressed && binding->keyboard_alt >= 0 && keyboard[binding->keyboard_alt]) {
            physical_pressed = true;
        }
        
        /* Check gamepad button binding */
        if (!physical_pressed && data->gamepad_enabled && binding->gamepad_button != CHIP8_GPAD_NONE) {
            physical_pressed = is_gamepad_button_pressed(data, binding->gamepad_button);
        }
        
        /* Handle analog stick as directional input (keys 2,4,6,8 = up,left,right,down) */
        if (!physical_pressed && data->gamepad_enabled && data->use_left_stick) {
            switch (key) {
                case 2: physical_pressed = stick_up; break;      /* Up */
                case 4: physical_pressed = stick_left; break;    /* Left */
                case 6: physical_pressed = stick_right; break;   /* Right */
                case 8: physical_pressed = stick_down; break;    /* Down */
                default: break;
            }
        }
        
        if (physical_pressed) {
            if (data->key_first_press[key]) {
                /* First press - register immediately */
                ctx->keys[key] = true;
                data->key_first_press[key] = false;
                data->key_repeat_time[key] = now;
                
                /* Optional: haptic feedback on first press */
                if (data->vibration_enabled && data->gamepad_enabled) {
                    gamepad_rumble(data, 0.2f, 30);
                }
            } else {
                /* Check if enough time has passed for repeat */
                uint64_t elapsed = now - data->key_repeat_time[key];
                uint64_t delay = (data->key_repeat_time[key] == 0) ? 
                                 data->key_repeat_delay_us : data->key_repeat_rate_us;
                
                if (elapsed >= delay) {
                    ctx->keys[key] = true;
                    data->key_repeat_time[key] = now;
                } else {
                    ctx->keys[key] = false;
                }
            }
        } else {
            ctx->keys[key] = false;
            data->key_first_press[key] = true;
        }
    }
}

static bool sdl_should_quit(Chip8Context* ctx) {
    SDLPlatformData* data = (SDLPlatformData*)ctx->platform_data;
    return data ? data->quit_requested : true;
}

static uint64_t sdl_get_time_us(void) {
    return SDL_GetPerformanceCounter() * 1000000 / SDL_GetPerformanceFrequency();
}

static void sdl_sleep_us(uint64_t microseconds) {
    SDL_Delay((uint32_t)(microseconds / 1000));
}

/* ============================================================================
 * Menu Input Handling
 * ========================================================================== */

static int sdl_poll_menu_events(Chip8Context* ctx) {
    SDLPlatformData* data = (SDLPlatformData*)ctx->platform_data;
    if (!data) return CHIP8_NAV_NONE;
    
    uint64_t now = sdl_get_time_us();
    int nav = CHIP8_NAV_NONE;
    
    /* Check if ESC was pressed during poll_events */
    if (data->escape_pressed) {
        data->escape_pressed = false;
        return CHIP8_NAV_BACK;
    }
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                data->quit_requested = true;
                ctx->running = false;
                return CHIP8_NAV_NONE;
                
            case SDL_KEYDOWN:
                if (event.key.repeat) continue; /* Ignore key repeat events */
                
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_UP:
                    case SDL_SCANCODE_W:
                        nav = CHIP8_NAV_UP;
                        break;
                    case SDL_SCANCODE_DOWN:
                    case SDL_SCANCODE_S:
                        nav = CHIP8_NAV_DOWN;
                        break;
                    case SDL_SCANCODE_LEFT:
                    case SDL_SCANCODE_A:
                        nav = CHIP8_NAV_LEFT;
                        break;
                    case SDL_SCANCODE_RIGHT:
                    case SDL_SCANCODE_D:
                        nav = CHIP8_NAV_RIGHT;
                        break;
                    case SDL_SCANCODE_RETURN:
                    case SDL_SCANCODE_SPACE:
                        nav = CHIP8_NAV_SELECT;
                        break;
                    case SDL_SCANCODE_ESCAPE:
                    case SDL_SCANCODE_BACKSPACE:
                        nav = CHIP8_NAV_BACK;
                        break;
                    default:
                        break;
                }
                
                if (nav != CHIP8_NAV_NONE) {
                    data->last_menu_nav = nav;
                    data->menu_repeat_time = now;
                }
                break;
                
            case SDL_KEYUP:
                /* Reset repeat on key release */
                data->last_menu_nav = CHIP8_NAV_NONE;
                data->menu_repeat_time = 0;
                break;
        }
    }
    
    /* Handle held keys for menu navigation repeat */
    if (nav == CHIP8_NAV_NONE && data->last_menu_nav != CHIP8_NAV_NONE) {
        const uint8_t* keyboard = SDL_GetKeyboardState(NULL);
        bool still_pressed = false;
        
        switch (data->last_menu_nav) {
            case CHIP8_NAV_UP:
                still_pressed = keyboard[SDL_SCANCODE_UP] || keyboard[SDL_SCANCODE_W];
                break;
            case CHIP8_NAV_DOWN:
                still_pressed = keyboard[SDL_SCANCODE_DOWN] || keyboard[SDL_SCANCODE_S];
                break;
            case CHIP8_NAV_LEFT:
                still_pressed = keyboard[SDL_SCANCODE_LEFT] || keyboard[SDL_SCANCODE_A];
                break;
            case CHIP8_NAV_RIGHT:
                still_pressed = keyboard[SDL_SCANCODE_RIGHT] || keyboard[SDL_SCANCODE_D];
                break;
            default:
                break;
        }
        
        if (still_pressed) {
            uint64_t elapsed = now - data->menu_repeat_time;
            if (elapsed >= MENU_REPEAT_DELAY_US) {
                nav = data->last_menu_nav;
                data->menu_repeat_time = now - MENU_REPEAT_DELAY_US + MENU_REPEAT_RATE_US;
            }
        } else {
            data->last_menu_nav = CHIP8_NAV_NONE;
        }
    }
    
    return nav;
}

/* ============================================================================
 * Menu Rendering
 * ========================================================================== */

/* Simple 5x7 bitmap font for menu text */
static const uint8_t MENU_FONT[95][7] = {
    /* Space (32) */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* ! (33) */
    {0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00},
    /* " (34) */
    {0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* # (35) */
    {0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00, 0x00},
    /* $ (36) */
    {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04},
    /* % (37) */
    {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03},
    /* & (38) */
    {0x08, 0x14, 0x08, 0x15, 0x12, 0x0D, 0x00},
    /* ' (39) */
    {0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* ( (40) */
    {0x02, 0x04, 0x04, 0x04, 0x04, 0x02, 0x00},
    /* ) (41) */
    {0x08, 0x04, 0x04, 0x04, 0x04, 0x08, 0x00},
    /* * (42) */
    {0x00, 0x0A, 0x04, 0x1F, 0x04, 0x0A, 0x00},
    /* + (43) */
    {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00},
    /* , (44) */
    {0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08},
    /* - (45) */
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},
    /* . (46) */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00},
    /* / (47) */
    {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00},
    /* 0-9 (48-57) */
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x0E, 0x00},
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x0E, 0x00},
    {0x0E, 0x11, 0x01, 0x0E, 0x10, 0x1F, 0x00},
    {0x0E, 0x11, 0x06, 0x01, 0x11, 0x0E, 0x00},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x00},
    {0x1F, 0x10, 0x1E, 0x01, 0x11, 0x0E, 0x00},
    {0x0E, 0x10, 0x1E, 0x11, 0x11, 0x0E, 0x00},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x00},
    {0x0E, 0x11, 0x0E, 0x11, 0x11, 0x0E, 0x00},
    {0x0E, 0x11, 0x0F, 0x01, 0x11, 0x0E, 0x00},
    /* : (58) */
    {0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00},
    /* ; (59) */
    {0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x08},
    /* < (60) */
    {0x01, 0x02, 0x04, 0x08, 0x04, 0x02, 0x01},
    /* = (61) */
    {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00},
    /* > (62) */
    {0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10},
    /* ? (63) */
    {0x0E, 0x11, 0x02, 0x04, 0x00, 0x04, 0x00},
    /* @ (64) */
    {0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E},
    /* A-Z (65-90) */
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x00},
    {0x1E, 0x11, 0x1E, 0x11, 0x11, 0x1E, 0x00},
    {0x0E, 0x11, 0x10, 0x10, 0x11, 0x0E, 0x00},
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x1E, 0x00},
    {0x1F, 0x10, 0x1E, 0x10, 0x10, 0x1F, 0x00},
    {0x1F, 0x10, 0x1E, 0x10, 0x10, 0x10, 0x00},
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x0F, 0x00},
    {0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00},
    {0x01, 0x01, 0x01, 0x01, 0x11, 0x0E, 0x00},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x1F, 0x00},
    {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x00},
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x00},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00},
    {0x1E, 0x11, 0x1E, 0x10, 0x10, 0x10, 0x00},
    {0x0E, 0x11, 0x11, 0x15, 0x12, 0x0D, 0x00},
    {0x1E, 0x11, 0x1E, 0x14, 0x12, 0x11, 0x00},
    {0x0E, 0x10, 0x0E, 0x01, 0x11, 0x0E, 0x00},
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00},
    {0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04, 0x00},
    {0x11, 0x11, 0x15, 0x15, 0x0A, 0x0A, 0x00},
    {0x11, 0x0A, 0x04, 0x04, 0x0A, 0x11, 0x00},
    {0x11, 0x0A, 0x04, 0x04, 0x04, 0x04, 0x00},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x1F, 0x00},
    /* [ (91) */
    {0x0E, 0x08, 0x08, 0x08, 0x08, 0x0E, 0x00},
    /* \ (92) */
    {0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00},
    /* ] (93) */
    {0x0E, 0x02, 0x02, 0x02, 0x02, 0x0E, 0x00},
    /* ^ (94) */
    {0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00},
    /* _ (95) */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00},
    /* ` (96) */
    {0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* a-z (97-122) */
    {0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F, 0x00},
    {0x10, 0x10, 0x1E, 0x11, 0x11, 0x1E, 0x00},
    {0x00, 0x0E, 0x10, 0x10, 0x10, 0x0E, 0x00},
    {0x01, 0x01, 0x0F, 0x11, 0x11, 0x0F, 0x00},
    {0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0x00},
    {0x06, 0x08, 0x1C, 0x08, 0x08, 0x08, 0x00},
    {0x00, 0x0F, 0x11, 0x0F, 0x01, 0x0E, 0x00},
    {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x00},
    {0x04, 0x00, 0x0C, 0x04, 0x04, 0x0E, 0x00},
    {0x02, 0x00, 0x06, 0x02, 0x12, 0x0C, 0x00},
    {0x10, 0x10, 0x12, 0x1C, 0x12, 0x11, 0x00},
    {0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00},
    {0x00, 0x1A, 0x15, 0x15, 0x11, 0x11, 0x00},
    {0x00, 0x1E, 0x11, 0x11, 0x11, 0x11, 0x00},
    {0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00},
    {0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10, 0x00},
    {0x00, 0x0F, 0x11, 0x0F, 0x01, 0x01, 0x00},
    {0x00, 0x16, 0x19, 0x10, 0x10, 0x10, 0x00},
    {0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E, 0x00},
    {0x08, 0x1C, 0x08, 0x08, 0x08, 0x06, 0x00},
    {0x00, 0x11, 0x11, 0x11, 0x11, 0x0F, 0x00},
    {0x00, 0x11, 0x11, 0x0A, 0x0A, 0x04, 0x00},
    {0x00, 0x11, 0x11, 0x15, 0x15, 0x0A, 0x00},
    {0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00},
    {0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E, 0x00},
    {0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F, 0x00},
};

static void draw_char(SDL_Renderer* renderer, int x, int y, char c, int scale) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t* glyph = MENU_FONT[c - 32];
    
    for (int row = 0; row < 7; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 5; col++) {
            if (bits & (0x10 >> col)) {
                SDL_Rect rect = {
                    x + col * scale,
                    y + row * scale,
                    scale,
                    scale
                };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}

static void draw_text(SDL_Renderer* renderer, int x, int y, const char* text, int scale) {
    int char_width = 6 * scale;
    while (*text) {
        draw_char(renderer, x, y, *text, scale);
        x += char_width;
        text++;
    }
}

static int text_width(const char* text, int scale) {
    return (int)strlen(text) * 6 * scale;
}

static void sdl_render_menu(Chip8Context* ctx, void* menu_ptr) {
    SDLPlatformData* data = (SDLPlatformData*)ctx->platform_data;
    Chip8MenuState* menu = (Chip8MenuState*)menu_ptr;
    if (!data || !menu) return;
    
    int win_w, win_h;
    SDL_GetWindowSize(data->window, &win_w, &win_h);
    
    /* Draw semi-transparent overlay */
    SDL_SetRenderDrawBlendMode(data->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, win_w, win_h};
    SDL_RenderFillRect(data->renderer, &overlay);
    
    /* Menu box */
    int box_w = 300;
    int box_h = 50 + menu->item_count * 30;
    int box_x = (win_w - box_w) / 2;
    int box_y = (win_h - box_h) / 2;
    
    /* Box background */
    SDL_SetRenderDrawColor(data->renderer, 30, 30, 40, 240);
    SDL_Rect box = {box_x, box_y, box_w, box_h};
    SDL_RenderFillRect(data->renderer, &box);
    
    /* Box border */
    SDL_SetRenderDrawColor(data->renderer, 100, 100, 120, 255);
    SDL_RenderDrawRect(data->renderer, &box);
    
    /* Title */
    const char* title = chip8_menu_get_title(menu);
    int text_scale = 2;
    int title_w = text_width(title, text_scale);
    SDL_SetRenderDrawColor(data->renderer, 255, 255, 255, 255);
    draw_text(data->renderer, box_x + (box_w - title_w) / 2, box_y + 12, title, text_scale);
    
    /* Menu items */
    int item_y = box_y + 45;
    for (int i = 0; i < menu->item_count; i++) {
        const char* label = chip8_menu_get_item_label(menu, i);
        const char* value = chip8_menu_get_item_value(menu, i);
        bool selected = chip8_menu_is_item_selected(menu, i);
        
        if (!label) continue;
        
        /* Highlight selected item */
        if (selected) {
            SDL_SetRenderDrawColor(data->renderer, 60, 60, 80, 255);
            SDL_Rect sel = {box_x + 5, item_y - 2, box_w - 10, 24};
            SDL_RenderFillRect(data->renderer, &sel);
            
            /* Selection indicator */
            SDL_SetRenderDrawColor(data->renderer, 100, 200, 100, 255);
            draw_text(data->renderer, box_x + 10, item_y, ">", text_scale);
        }
        
        /* Item label */
        SDL_SetRenderDrawColor(data->renderer, selected ? 255 : 200, 
                                               selected ? 255 : 200, 
                                               selected ? 255 : 200, 255);
        draw_text(data->renderer, box_x + 25, item_y, label, text_scale);
        
        /* Item value (if any) */
        if (value) {
            int val_w = text_width(value, text_scale);
            SDL_SetRenderDrawColor(data->renderer, 150, 200, 255, 255);
            draw_text(data->renderer, box_x + box_w - val_w - 15, item_y, value, text_scale);
        }
        
        item_y += 26;
    }
    
    /* Controls hint */
    SDL_SetRenderDrawColor(data->renderer, 120, 120, 120, 255);
    const char* hint = "Arrow Keys: Navigate  Enter: Select  Esc: Back";
    int hint_w = text_width(hint, 1);
    draw_text(data->renderer, (win_w - hint_w) / 2, win_h - 20, hint, 1);
    
    SDL_RenderPresent(data->renderer);
}

/* ============================================================================
 * Settings Application
 * ========================================================================== */

static void sdl_apply_settings(Chip8Context* ctx, void* settings_ptr) {
    SDLPlatformData* data = (SDLPlatformData*)ctx->platform_data;
    Chip8Settings* settings = (Chip8Settings*)settings_ptr;
    if (!data || !settings) return;
    
    /* Apply audio settings */
    data->audio_volume = settings->audio.muted ? 0.0f : settings->audio.volume;
    data->audio_frequency = settings->audio.frequency;
    data->audio_waveform = settings->audio.waveform;
    
    /* Apply color theme */
    Chip8ThemeColors colors;
    if (settings->graphics.theme == CHIP8_THEME_CUSTOM) {
        colors.fg = settings->graphics.custom_fg;
        colors.bg = settings->graphics.custom_bg;
    } else {
        colors = chip8_get_theme_colors(settings->graphics.theme);
    }
    data->fg_color = colors.fg;
    data->bg_color = colors.bg;
    
    /* Apply key repeat settings */
    data->key_repeat_delay_us = settings->gameplay.key_repeat_delay_ms * 1000;
    data->key_repeat_rate_us = settings->gameplay.key_repeat_rate_ms * 1000;
    
    /* Apply window scale if changed */
    if (data->scale != settings->graphics.scale) {
        data->scale = settings->graphics.scale;
        int width = CHIP8_DISPLAY_WIDTH * data->scale;
        int height = CHIP8_DISPLAY_HEIGHT * data->scale;
        SDL_SetWindowSize(data->window, width, height);
        SDL_SetWindowPosition(data->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    
    /* Apply fullscreen */
    Uint32 flags = settings->graphics.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(data->window, flags);
    
    /* Apply visual effects */
    data->pixel_grid = settings->graphics.pixel_grid;
    data->crt_effect = settings->graphics.crt_effect;
    data->scanline_intensity = settings->graphics.scanline_intensity;
    
    /* Apply input settings */
    memcpy(data->key_bindings, settings->input.bindings, sizeof(data->key_bindings));
    data->gamepad_enabled = settings->input.gamepad_enabled;
    data->active_gamepad_idx = settings->input.active_gamepad;
    data->analog_deadzone = settings->input.analog_deadzone;
    data->use_left_stick = settings->input.use_left_stick;
    data->use_dpad = settings->input.use_dpad;
    data->vibration_enabled = settings->input.vibration_enabled;
    data->vibration_intensity = settings->input.vibration_intensity;
    
    /* Store settings reference for ImGui overlay */
    data->settings_ref = settings;
    
    /* Force display redraw */
    ctx->display_dirty = true;
}

/* ============================================================================
 * Platform Export
 * ========================================================================== */

static Chip8Platform sdl_platform = {
    .name           = "SDL2",
    .init           = sdl_init,
    .shutdown       = sdl_shutdown,
    .render         = sdl_render,
    .beep_start     = sdl_beep_start,
    .beep_stop      = sdl_beep_stop,
    .poll_events    = sdl_poll_events,
    .poll_menu_events = sdl_poll_menu_events,
    .should_quit    = sdl_should_quit,
    .render_menu    = sdl_render_menu,
    .apply_settings = sdl_apply_settings,
    .get_time_us    = sdl_get_time_us,
    .sleep_us       = sdl_sleep_us,
};

Chip8Platform* chip8_platform_sdl2(void) {
    return &sdl_platform;
}
