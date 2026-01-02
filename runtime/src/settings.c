/**
 * @file settings.c
 * @brief Runtime settings implementation
 */

#include "chip8rt/settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================================
 * Theme Color Definitions
 * ========================================================================== */

static const Chip8ThemeColors g_theme_colors[CHIP8_THEME_COUNT] = {
    /* CLASSIC - White on black */
    { .fg = {255, 255, 255, 255}, .bg = {0, 0, 0, 255} },
    
    /* GREEN_PHOSPHOR - P1 phosphor green */
    { .fg = {51, 255, 51, 255}, .bg = {0, 32, 0, 255} },
    
    /* AMBER - Warm amber CRT */
    { .fg = {255, 176, 0, 255}, .bg = {32, 16, 0, 255} },
    
    /* LCD - Classic LCD gray on dark green */
    { .fg = {67, 82, 61, 255}, .bg = {155, 188, 15, 255} },
    
    /* CUSTOM - defaults to white on black, overridden by settings */
    { .fg = {255, 255, 255, 255}, .bg = {0, 0, 0, 255} },
};

static const char* g_theme_names[CHIP8_THEME_COUNT] = {
    "Classic",
    "Green Phosphor",
    "Amber",
    "LCD",
    "Custom"
};

static const char* g_waveform_names[CHIP8_WAVE_COUNT] = {
    "Square",
    "Sine",
    "Triangle",
    "Sawtooth",
    "Noise"
};

static const char* g_window_size_names[CHIP8_WINDOW_COUNT] = {
    "1x (64x32)",
    "2x (128x64)",
    "5x (320x160)",
    "10x (640x320)",
    "15x (960x480)",
    "20x (1280x640)",
    "Custom"
};

static const int g_window_size_scales[CHIP8_WINDOW_COUNT] = {
    1, 2, 5, 10, 15, 20, 10  /* Custom defaults to 10 */
};

/* Default keyboard scancodes for CHIP-8 keys (SDL scancodes) */
/* CHIP-8:  1 2 3 C    Keyboard: 1 2 3 4  */
/*          4 5 6 D              Q W E R  */
/*          7 8 9 E              A S D F  */
/*          A 0 B F              Z X C V  */
static const int32_t g_default_key_scancodes[16] = {
    27,  /* 0 -> X (SDL_SCANCODE_X) */
    30,  /* 1 -> 1 (SDL_SCANCODE_1) */
    31,  /* 2 -> 2 (SDL_SCANCODE_2) */
    32,  /* 3 -> 3 (SDL_SCANCODE_3) */
    20,  /* 4 -> Q (SDL_SCANCODE_Q) */
    26,  /* 5 -> W (SDL_SCANCODE_W) */
    8,   /* 6 -> E (SDL_SCANCODE_E) */
    4,   /* 7 -> A (SDL_SCANCODE_A) */
    22,  /* 8 -> S (SDL_SCANCODE_S) */
    7,   /* 9 -> D (SDL_SCANCODE_D) */
    29,  /* A -> Z (SDL_SCANCODE_Z) */
    6,   /* B -> C (SDL_SCANCODE_C) */
    33,  /* C -> 4 (SDL_SCANCODE_4) */
    21,  /* D -> R (SDL_SCANCODE_R) */
    9,   /* E -> F (SDL_SCANCODE_F) */
    25   /* F -> V (SDL_SCANCODE_V) */
};

/* Default gamepad button mappings - common layout for grid games */
static const Chip8GamepadButton g_default_gamepad_buttons[16] = {
    CHIP8_GPAD_A,            /* 0 - Action button */
    CHIP8_GPAD_NONE,         /* 1 */
    CHIP8_GPAD_DPAD_UP,      /* 2 - Up */
    CHIP8_GPAD_NONE,         /* 3 */
    CHIP8_GPAD_DPAD_LEFT,    /* 4 - Left */
    CHIP8_GPAD_B,            /* 5 - Secondary action */
    CHIP8_GPAD_DPAD_RIGHT,   /* 6 - Right */
    CHIP8_GPAD_NONE,         /* 7 */
    CHIP8_GPAD_DPAD_DOWN,    /* 8 - Down */
    CHIP8_GPAD_NONE,         /* 9 */
    CHIP8_GPAD_X,            /* A */
    CHIP8_GPAD_Y,            /* B */
    CHIP8_GPAD_LEFTSHOULDER, /* C */
    CHIP8_GPAD_RIGHTSHOULDER,/* D */
    CHIP8_GPAD_START,        /* E */
    CHIP8_GPAD_BACK          /* F */
};

static const char* g_gamepad_button_names[CHIP8_GPAD_BUTTON_COUNT] = {
    "A", "B", "X", "Y",
    "Back", "Guide", "Start",
    "Left Stick", "Right Stick",
    "Left Shoulder", "Right Shoulder",
    "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right"
};

static const char* g_chip8_key_labels[16] = {
    "0", "1", "2", "3", "4", "5", "6", "7",
    "8", "9", "A", "B", "C", "D", "E", "F"
};

/* ============================================================================
 * Default Settings
 * ========================================================================== */

void chip8_input_settings_default(Chip8InputSettings* input) {
    if (!input) return;
    
    /* Initialize all bindings */
    for (int i = 0; i < 16; i++) {
        input->bindings[i].keyboard = g_default_key_scancodes[i];
        input->bindings[i].keyboard_alt = -1;  /* No alternate binding */
        input->bindings[i].gamepad_button = g_default_gamepad_buttons[i];
    }
    
    /* Default gamepad settings */
    input->gamepad_enabled = true;
    input->active_gamepad = 0;
    input->analog_deadzone = 0.25f;
    input->use_left_stick = true;
    input->use_dpad = true;
    input->vibration_enabled = true;
    input->vibration_intensity = 0.5f;
}

Chip8Settings chip8_settings_default(void) {
    Chip8Settings settings = {
        .graphics = {
            .window_size = CHIP8_WINDOW_10X,
            .scale = 10,
            .fullscreen = false,
            .theme = CHIP8_THEME_CLASSIC,
            .custom_fg = {255, 255, 255, 255},
            .custom_bg = {0, 0, 0, 255},
            .pixel_grid = false,
            .crt_effect = false,
            .scanline_intensity = 0.2f,
            .screen_curve = false
        },
        .audio = {
            .volume = 0.5f,
            .frequency = 440,
            .waveform = CHIP8_WAVE_SQUARE,
            .muted = false
        },
        .gameplay = {
            .cpu_freq_hz = 700,
            .key_repeat_delay_ms = 200,
            .key_repeat_rate_ms = 100,
            .quirks = {
                .vf_reset = false,
                .shift_uses_vy = false,
                .memory_increment_i = true,
                .sprite_wrap = false,
                .jump_uses_vx = false,
                .display_wait = true
            }
        },
        .input = {0}
    };
    
    /* Initialize input settings with defaults */
    chip8_input_settings_default(&settings.input);
    
    return settings;
}

/* ============================================================================
 * Theme/Waveform Names
 * ========================================================================== */

Chip8ThemeColors chip8_get_theme_colors(Chip8ColorTheme theme) {
    if (theme >= 0 && theme < CHIP8_THEME_COUNT) {
        return g_theme_colors[theme];
    }
    return g_theme_colors[CHIP8_THEME_CLASSIC];
}

const char* chip8_get_theme_name(Chip8ColorTheme theme) {
    if (theme >= 0 && theme < CHIP8_THEME_COUNT) {
        return g_theme_names[theme];
    }
    return "Unknown";
}

const char* chip8_get_waveform_name(Chip8Waveform waveform) {
    if (waveform >= 0 && waveform < CHIP8_WAVE_COUNT) {
        return g_waveform_names[waveform];
    }
    return "Unknown";
}

const char* chip8_get_window_size_name(Chip8WindowSize size) {
    if (size >= 0 && size < CHIP8_WINDOW_COUNT) {
        return g_window_size_names[size];
    }
    return "Unknown";
}

int chip8_get_window_size_scale(Chip8WindowSize size) {
    if (size >= 0 && size < CHIP8_WINDOW_COUNT) {
        return g_window_size_scales[size];
    }
    return 10;  /* Default to 10x */
}

/* ============================================================================
 * Config File Parsing Helpers
 * ========================================================================== */

static void skip_whitespace(const char** p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

static bool parse_bool(const char* value) {
    return (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
            strcmp(value, "yes") == 0 || strcmp(value, "on") == 0);
}

static int parse_int(const char* value, int min_val, int max_val, int default_val) {
    char* end;
    long v = strtol(value, &end, 10);
    if (end == value) return default_val;
    if (v < min_val) return min_val;
    if (v > max_val) return max_val;
    return (int)v;
}

static float parse_float(const char* value, float min_val, float max_val, float default_val) {
    char* end;
    float v = strtof(value, &end);
    if (end == value) return default_val;
    if (v < min_val) return min_val;
    if (v > max_val) return max_val;
    return v;
}

static Chip8Color parse_color(const char* value) {
    Chip8Color color = {255, 255, 255, 255};
    unsigned int r, g, b;
    if (sscanf(value, "#%02x%02x%02x", &r, &g, &b) == 3 ||
        sscanf(value, "%02x%02x%02x", &r, &g, &b) == 3) {
        color.r = (uint8_t)r;
        color.g = (uint8_t)g;
        color.b = (uint8_t)b;
    }
    return color;
}

static Chip8ColorTheme parse_theme(const char* value) {
    for (int i = 0; i < CHIP8_THEME_COUNT; i++) {
        if (strcasecmp(value, g_theme_names[i]) == 0) {
            return (Chip8ColorTheme)i;
        }
    }
    /* Try lowercase with underscores */
    if (strcasecmp(value, "classic") == 0) return CHIP8_THEME_CLASSIC;
    if (strcasecmp(value, "green_phosphor") == 0) return CHIP8_THEME_GREEN_PHOSPHOR;
    if (strcasecmp(value, "amber") == 0) return CHIP8_THEME_AMBER;
    if (strcasecmp(value, "lcd") == 0) return CHIP8_THEME_LCD;
    if (strcasecmp(value, "custom") == 0) return CHIP8_THEME_CUSTOM;
    return CHIP8_THEME_CLASSIC;
}

static Chip8Waveform parse_waveform(const char* value) {
    for (int i = 0; i < CHIP8_WAVE_COUNT; i++) {
        if (strcasecmp(value, g_waveform_names[i]) == 0) {
            return (Chip8Waveform)i;
        }
    }
    return CHIP8_WAVE_SQUARE;
}

/* ============================================================================
 * Config File Load/Save
 * ========================================================================== */

bool chip8_settings_load(Chip8Settings* settings, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        return false;
    }
    
    /* Start with defaults */
    *settings = chip8_settings_default();
    
    char line[256];
    char section[64] = "";
    
    while (fgets(line, sizeof(line), f)) {
        /* Remove newline */
        char* nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        
        const char* p = line;
        skip_whitespace(&p);
        
        /* Skip empty lines and comments */
        if (*p == '\0' || *p == '#' || *p == ';') continue;
        
        /* Section header */
        if (*p == '[') {
            p++;
            char* end = strchr(p, ']');
            if (end) {
                *end = '\0';
                strncpy(section, p, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';
            }
            continue;
        }
        
        /* Key = Value */
        char key[64] = "";
        char value[128] = "";
        
        char* eq = strchr(p, '=');
        if (!eq) continue;
        
        /* Extract key */
        const char* key_start = p;
        const char* key_end = eq;
        while (key_end > key_start && isspace((unsigned char)*(key_end - 1))) key_end--;
        size_t key_len = key_end - key_start;
        if (key_len >= sizeof(key)) key_len = sizeof(key) - 1;
        strncpy(key, key_start, key_len);
        key[key_len] = '\0';
        
        /* Extract value */
        const char* val_start = eq + 1;
        while (*val_start && isspace((unsigned char)*val_start)) val_start++;
        strncpy(value, val_start, sizeof(value) - 1);
        value[sizeof(value) - 1] = '\0';
        /* Trim trailing whitespace */
        char* val_end = value + strlen(value);
        while (val_end > value && isspace((unsigned char)*(val_end - 1))) val_end--;
        *val_end = '\0';
        
        /* Apply settings based on section and key */
        if (strcmp(section, "graphics") == 0) {
            if (strcmp(key, "window_size") == 0) {
                settings->graphics.window_size = parse_int(value, 0, CHIP8_WINDOW_COUNT - 1, CHIP8_WINDOW_10X);
            } else if (strcmp(key, "scale") == 0) {
                settings->graphics.scale = parse_int(value, 1, 20, 10);
            } else if (strcmp(key, "fullscreen") == 0) {
                settings->graphics.fullscreen = parse_bool(value);
            } else if (strcmp(key, "theme") == 0) {
                settings->graphics.theme = parse_theme(value);
            } else if (strcmp(key, "custom_fg") == 0) {
                settings->graphics.custom_fg = parse_color(value);
            } else if (strcmp(key, "custom_bg") == 0) {
                settings->graphics.custom_bg = parse_color(value);
            } else if (strcmp(key, "pixel_grid") == 0) {
                settings->graphics.pixel_grid = parse_bool(value);
            } else if (strcmp(key, "crt_effect") == 0) {
                settings->graphics.crt_effect = parse_bool(value);
            } else if (strcmp(key, "scanline_intensity") == 0) {
                settings->graphics.scanline_intensity = parse_float(value, 0.0f, 1.0f, 0.2f);
            } else if (strcmp(key, "screen_curve") == 0) {
                settings->graphics.screen_curve = parse_bool(value);
            }
        } else if (strcmp(section, "audio") == 0) {
            if (strcmp(key, "volume") == 0) {
                settings->audio.volume = parse_float(value, 0.0f, 1.0f, 0.5f);
            } else if (strcmp(key, "frequency") == 0) {
                settings->audio.frequency = parse_int(value, 220, 880, 440);
            } else if (strcmp(key, "waveform") == 0) {
                settings->audio.waveform = parse_waveform(value);
            } else if (strcmp(key, "muted") == 0) {
                settings->audio.muted = parse_bool(value);
            }
        } else if (strcmp(section, "gameplay") == 0) {
            if (strcmp(key, "cpu_freq_hz") == 0) {
                settings->gameplay.cpu_freq_hz = parse_int(value, 100, 2000, 700);
            } else if (strcmp(key, "key_repeat_delay_ms") == 0) {
                settings->gameplay.key_repeat_delay_ms = parse_int(value, 100, 1000, 200);
            } else if (strcmp(key, "key_repeat_rate_ms") == 0) {
                settings->gameplay.key_repeat_rate_ms = parse_int(value, 50, 500, 100);
            }
        } else if (strcmp(section, "quirks") == 0) {
            if (strcmp(key, "vf_reset") == 0) {
                settings->gameplay.quirks.vf_reset = parse_bool(value);
            } else if (strcmp(key, "shift_uses_vy") == 0) {
                settings->gameplay.quirks.shift_uses_vy = parse_bool(value);
            } else if (strcmp(key, "memory_increment_i") == 0) {
                settings->gameplay.quirks.memory_increment_i = parse_bool(value);
            } else if (strcmp(key, "sprite_wrap") == 0) {
                settings->gameplay.quirks.sprite_wrap = parse_bool(value);
            } else if (strcmp(key, "jump_uses_vx") == 0) {
                settings->gameplay.quirks.jump_uses_vx = parse_bool(value);
            } else if (strcmp(key, "display_wait") == 0) {
                settings->gameplay.quirks.display_wait = parse_bool(value);
            }
        } else if (strcmp(section, "input") == 0) {
            /* Input settings */
            if (strcmp(key, "gamepad_enabled") == 0) {
                settings->input.gamepad_enabled = parse_bool(value);
            } else if (strcmp(key, "active_gamepad") == 0) {
                settings->input.active_gamepad = parse_int(value, 0, CHIP8_MAX_GAMEPADS - 1, 0);
            } else if (strcmp(key, "analog_deadzone") == 0) {
                settings->input.analog_deadzone = parse_float(value, 0.0f, 1.0f, 0.25f);
            } else if (strcmp(key, "use_left_stick") == 0) {
                settings->input.use_left_stick = parse_bool(value);
            } else if (strcmp(key, "use_dpad") == 0) {
                settings->input.use_dpad = parse_bool(value);
            } else if (strcmp(key, "vibration_enabled") == 0) {
                settings->input.vibration_enabled = parse_bool(value);
            } else if (strcmp(key, "vibration_intensity") == 0) {
                settings->input.vibration_intensity = parse_float(value, 0.0f, 1.0f, 0.5f);
            }
        } else if (strncmp(section, "keybind_", 8) == 0) {
            /* Key binding for specific key: [keybind_0] through [keybind_F] */
            int key_idx = -1;
            char hex_char = section[8];
            if (hex_char >= '0' && hex_char <= '9') {
                key_idx = hex_char - '0';
            } else if (hex_char >= 'A' && hex_char <= 'F') {
                key_idx = 10 + (hex_char - 'A');
            } else if (hex_char >= 'a' && hex_char <= 'f') {
                key_idx = 10 + (hex_char - 'a');
            }
            
            if (key_idx >= 0 && key_idx < 16) {
                if (strcmp(key, "keyboard") == 0) {
                    settings->input.bindings[key_idx].keyboard = parse_int(value, -1, 512, -1);
                } else if (strcmp(key, "keyboard_alt") == 0) {
                    settings->input.bindings[key_idx].keyboard_alt = parse_int(value, -1, 512, -1);
                } else if (strcmp(key, "gamepad") == 0) {
                    settings->input.bindings[key_idx].gamepad_button = (Chip8GamepadButton)parse_int(value, -1, CHIP8_GPAD_BUTTON_COUNT - 1, -1);
                }
            }
        }
    }
    
    fclose(f);
    return true;
}

bool chip8_settings_save(const Chip8Settings* settings, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) {
        return false;
    }
    
    fprintf(f, "# CHIP-8 Recompiled Settings\n\n");
    
    /* Graphics */
    fprintf(f, "[graphics]\n");
    fprintf(f, "window_size = %d\n", settings->graphics.window_size);
    fprintf(f, "scale = %d\n", settings->graphics.scale);
    fprintf(f, "fullscreen = %s\n", settings->graphics.fullscreen ? "true" : "false");
    fprintf(f, "theme = %s\n", g_theme_names[settings->graphics.theme]);
    fprintf(f, "custom_fg = #%02x%02x%02x\n", 
            settings->graphics.custom_fg.r,
            settings->graphics.custom_fg.g,
            settings->graphics.custom_fg.b);
    fprintf(f, "custom_bg = #%02x%02x%02x\n",
            settings->graphics.custom_bg.r,
            settings->graphics.custom_bg.g,
            settings->graphics.custom_bg.b);
    fprintf(f, "pixel_grid = %s\n", settings->graphics.pixel_grid ? "true" : "false");
    fprintf(f, "crt_effect = %s\n", settings->graphics.crt_effect ? "true" : "false");
    fprintf(f, "scanline_intensity = %.2f\n", settings->graphics.scanline_intensity);
    fprintf(f, "screen_curve = %s\n", settings->graphics.screen_curve ? "true" : "false");
    fprintf(f, "\n");
    
    /* Audio */
    fprintf(f, "[audio]\n");
    fprintf(f, "volume = %.2f\n", settings->audio.volume);
    fprintf(f, "frequency = %d\n", settings->audio.frequency);
    fprintf(f, "waveform = %s\n", g_waveform_names[settings->audio.waveform]);
    fprintf(f, "muted = %s\n", settings->audio.muted ? "true" : "false");
    fprintf(f, "\n");
    
    /* Gameplay */
    fprintf(f, "[gameplay]\n");
    fprintf(f, "cpu_freq_hz = %d\n", settings->gameplay.cpu_freq_hz);
    fprintf(f, "key_repeat_delay_ms = %d\n", settings->gameplay.key_repeat_delay_ms);
    fprintf(f, "key_repeat_rate_ms = %d\n", settings->gameplay.key_repeat_rate_ms);
    fprintf(f, "\n");
    
    /* Quirks */
    fprintf(f, "[quirks]\n");
    fprintf(f, "vf_reset = %s\n", settings->gameplay.quirks.vf_reset ? "true" : "false");
    fprintf(f, "shift_uses_vy = %s\n", settings->gameplay.quirks.shift_uses_vy ? "true" : "false");
    fprintf(f, "memory_increment_i = %s\n", settings->gameplay.quirks.memory_increment_i ? "true" : "false");
    fprintf(f, "sprite_wrap = %s\n", settings->gameplay.quirks.sprite_wrap ? "true" : "false");
    fprintf(f, "jump_uses_vx = %s\n", settings->gameplay.quirks.jump_uses_vx ? "true" : "false");
    fprintf(f, "display_wait = %s\n", settings->gameplay.quirks.display_wait ? "true" : "false");
    fprintf(f, "\n");
    
    /* Input Settings */
    fprintf(f, "[input]\n");
    fprintf(f, "gamepad_enabled = %s\n", settings->input.gamepad_enabled ? "true" : "false");
    fprintf(f, "active_gamepad = %d\n", settings->input.active_gamepad);
    fprintf(f, "analog_deadzone = %.2f\n", settings->input.analog_deadzone);
    fprintf(f, "use_left_stick = %s\n", settings->input.use_left_stick ? "true" : "false");
    fprintf(f, "use_dpad = %s\n", settings->input.use_dpad ? "true" : "false");
    fprintf(f, "vibration_enabled = %s\n", settings->input.vibration_enabled ? "true" : "false");
    fprintf(f, "vibration_intensity = %.2f\n", settings->input.vibration_intensity);
    fprintf(f, "\n");
    
    /* Key Bindings */
    fprintf(f, "# Key bindings for each CHIP-8 key (0-F)\n");
    fprintf(f, "# keyboard/keyboard_alt = SDL scancode, gamepad = button index\n");
    for (int i = 0; i < 16; i++) {
        const char* hex = "0123456789ABCDEF";
        fprintf(f, "[keybind_%c]\n", hex[i]);
        fprintf(f, "keyboard = %d\n", settings->input.bindings[i].keyboard);
        fprintf(f, "keyboard_alt = %d\n", settings->input.bindings[i].keyboard_alt);
        fprintf(f, "gamepad = %d\n", (int)settings->input.bindings[i].gamepad_button);
        fprintf(f, "\n");
    }
    
    fclose(f);
    return true;
}

/* ============================================================================
 * Default Config Path
 * ========================================================================== */

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

const char* chip8_settings_get_default_path(void) {
    static char path_buffer[512];
    
#ifdef _WIN32
    /* Windows: use %APPDATA%\chip8recompiled\settings.ini */
    char appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
        snprintf(path_buffer, sizeof(path_buffer), "%s\\chip8recompiled\\settings.ini", appdata);
        
        /* Create directory if it doesn't exist */
        char dir[MAX_PATH];
        snprintf(dir, sizeof(dir), "%s\\chip8recompiled", appdata);
        CreateDirectoryA(dir, NULL);
        
        return path_buffer;
    }
#else
    /* Unix-like: use ~/.chip8recompiled/settings.ini */
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }
    
    if (home) {
        snprintf(path_buffer, sizeof(path_buffer), "%s/.chip8recompiled/settings.ini", home);
        
        /* Create directory if it doesn't exist */
        char dir[256];
        snprintf(dir, sizeof(dir), "%s/.chip8recompiled", home);
        mkdir(dir, 0755);
        
        return path_buffer;
    }
#endif
    
    /* Fallback to current directory */
    snprintf(path_buffer, sizeof(path_buffer), "chip8_settings.ini");
    return path_buffer;
}

/* Sanitize a filename by replacing invalid characters */
static void sanitize_filename(char* dest, const char* src, size_t max_len) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j < max_len - 1; ++i) {
        char c = src[i];
        /* Replace invalid filename characters with underscore */
        if (c == '/' || c == '\\' || c == ':' || c == '*' || 
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
        /* Skip leading dots */
        if (j == 0 && c == '.') continue;
        dest[j++] = c;
    }
    dest[j] = '\0';
    
    /* Ensure not empty */
    if (j == 0) {
        strncpy(dest, "default", max_len - 1);
        dest[max_len - 1] = '\0';
    }
}

const char* chip8_settings_get_rom_path(const char* rom_name) {
    static char path_buffer[512];
    
    if (!rom_name || !*rom_name) {
        return chip8_settings_get_default_path();  /* Fall back to global settings */
    }
    
    /* Sanitize the ROM name for use as a filename */
    char safe_name[128];
    sanitize_filename(safe_name, rom_name, sizeof(safe_name));
    
#ifdef _WIN32
    char appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
        /* Create directories */
        char dir[MAX_PATH];
        snprintf(dir, sizeof(dir), "%s\\chip8recompiled", appdata);
        CreateDirectoryA(dir, NULL);
        snprintf(dir, sizeof(dir), "%s\\chip8recompiled\\games", appdata);
        CreateDirectoryA(dir, NULL);
        
        snprintf(path_buffer, sizeof(path_buffer), "%s\\chip8recompiled\\games\\%s.ini", 
                 appdata, safe_name);
        return path_buffer;
    }
#else
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }
    
    if (home) {
        /* Create directories */
        char dir[256];
        snprintf(dir, sizeof(dir), "%s/.chip8recompiled", home);
        mkdir(dir, 0755);
        snprintf(dir, sizeof(dir), "%s/.chip8recompiled/games", home);
        mkdir(dir, 0755);
        
        snprintf(path_buffer, sizeof(path_buffer), "%s/.chip8recompiled/games/%s.ini", 
                 home, safe_name);
        return path_buffer;
    }
#endif
    
    /* Fallback to current directory */
    snprintf(path_buffer, sizeof(path_buffer), "%s_settings.ini", safe_name);
    return path_buffer;
}

/* ============================================================================
 * Input Helper Functions
 * ========================================================================== */

const char* chip8_get_gamepad_button_name(Chip8GamepadButton button) {
    if (button < 0 || button >= CHIP8_GPAD_BUTTON_COUNT) {
        return "None";
    }
    return g_gamepad_button_names[button];
}

const char* chip8_get_key_label(int key) {
    if (key < 0 || key >= 16) {
        return "?";
    }
    return g_chip8_key_labels[key];
}

/* SDL scancode names - common keys for mapping */
const char* chip8_get_scancode_name(Chip8Scancode scancode) {
    /* This returns a descriptive name for common scancodes */
    /* Note: In a real implementation, you'd use SDL_GetScancodeName */
    /* but we provide fallback names for common keys */
    static char buffer[32];
    
    switch (scancode) {
        case -1: return "None";
        case 4: return "A";
        case 5: return "B";
        case 6: return "C";
        case 7: return "D";
        case 8: return "E";
        case 9: return "F";
        case 10: return "G";
        case 11: return "H";
        case 12: return "I";
        case 13: return "J";
        case 14: return "K";
        case 15: return "L";
        case 16: return "M";
        case 17: return "N";
        case 18: return "O";
        case 19: return "P";
        case 20: return "Q";
        case 21: return "R";
        case 22: return "S";
        case 23: return "T";
        case 24: return "U";
        case 25: return "V";
        case 26: return "W";
        case 27: return "X";
        case 28: return "Y";
        case 29: return "Z";
        case 30: return "1";
        case 31: return "2";
        case 32: return "3";
        case 33: return "4";
        case 34: return "5";
        case 35: return "6";
        case 36: return "7";
        case 37: return "8";
        case 38: return "9";
        case 39: return "0";
        case 40: return "Return";
        case 41: return "Escape";
        case 42: return "Backspace";
        case 43: return "Tab";
        case 44: return "Space";
        case 79: return "Right";
        case 80: return "Left";
        case 81: return "Down";
        case 82: return "Up";
        default:
            snprintf(buffer, sizeof(buffer), "Key %d", scancode);
            return buffer;
    }
}
