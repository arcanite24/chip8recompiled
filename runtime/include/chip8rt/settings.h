/**
 * @file settings.h
 * @brief Runtime settings and configuration for CHIP-8
 * 
 * This header defines configurable options for graphics, audio,
 * gameplay, and input settings.
 */

#ifndef CHIP8RT_SETTINGS_H
#define CHIP8RT_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Window Size Presets
 * ========================================================================== */

/**
 * @brief Predefined window size presets
 */
typedef enum Chip8WindowSize {
    CHIP8_WINDOW_1X,    /* 64x32 - Original size */
    CHIP8_WINDOW_2X,    /* 128x64 */
    CHIP8_WINDOW_5X,    /* 320x160 */
    CHIP8_WINDOW_10X,   /* 640x320 - Default */
    CHIP8_WINDOW_15X,   /* 960x480 */
    CHIP8_WINDOW_20X,   /* 1280x640 */
    CHIP8_WINDOW_CUSTOM,/* Custom scale value */
    CHIP8_WINDOW_COUNT
} Chip8WindowSize;

/* ============================================================================
 * Color Themes
 * ========================================================================== */

/**
 * @brief Predefined color themes for display
 */
typedef enum Chip8ColorTheme {
    CHIP8_THEME_CLASSIC,       /* White on black */
    CHIP8_THEME_GREEN_PHOSPHOR,/* Green CRT phosphor */
    CHIP8_THEME_AMBER,         /* Amber CRT monitor */
    CHIP8_THEME_LCD,           /* LCD gray/dark green */
    CHIP8_THEME_CUSTOM,        /* User-defined colors */
    CHIP8_THEME_COUNT
} Chip8ColorTheme;

/**
 * @brief RGBA color value
 */
typedef struct Chip8Color {
    uint8_t r, g, b, a;
} Chip8Color;

/**
 * @brief Predefined theme colors (foreground, background)
 */
typedef struct Chip8ThemeColors {
    Chip8Color fg;  /* Foreground (pixel on) */
    Chip8Color bg;  /* Background (pixel off) */
} Chip8ThemeColors;

/* ============================================================================
 * Audio Waveforms
 * ========================================================================== */

/**
 * @brief Audio waveform types for beep sound
 */
typedef enum Chip8Waveform {
    CHIP8_WAVE_SQUARE,     /* Classic harsh beep */
    CHIP8_WAVE_SINE,       /* Smooth sine wave */
    CHIP8_WAVE_TRIANGLE,   /* Softer triangle wave */
    CHIP8_WAVE_SAWTOOTH,   /* Buzzy sawtooth */
    CHIP8_WAVE_NOISE,      /* White noise */
    CHIP8_WAVE_COUNT
} Chip8Waveform;

/* ============================================================================
 * CHIP-8 Quirks
 * ========================================================================== */

/* ============================================================================
 * Input/Keybinding Configuration
 * ========================================================================== */

/**
 * @brief Maximum number of supported gamepads
 */
#define CHIP8_MAX_GAMEPADS 4

/**
 * @brief Keyboard scancode for key mapping (uses SDL scancodes)
 */
typedef int32_t Chip8Scancode;

/**
 * @brief Gamepad button types for mapping
 */
typedef enum Chip8GamepadButton {
    CHIP8_GPAD_NONE = -1,
    CHIP8_GPAD_A = 0,
    CHIP8_GPAD_B,
    CHIP8_GPAD_X,
    CHIP8_GPAD_Y,
    CHIP8_GPAD_BACK,
    CHIP8_GPAD_GUIDE,
    CHIP8_GPAD_START,
    CHIP8_GPAD_LEFTSTICK,
    CHIP8_GPAD_RIGHTSTICK,
    CHIP8_GPAD_LEFTSHOULDER,
    CHIP8_GPAD_RIGHTSHOULDER,
    CHIP8_GPAD_DPAD_UP,
    CHIP8_GPAD_DPAD_DOWN,
    CHIP8_GPAD_DPAD_LEFT,
    CHIP8_GPAD_DPAD_RIGHT,
    CHIP8_GPAD_BUTTON_COUNT
} Chip8GamepadButton;

/**
 * @brief Key binding for a single CHIP-8 key
 * 
 * Supports both keyboard and gamepad bindings.
 */
typedef struct Chip8KeyBinding {
    /** Primary keyboard scancode (-1 if unbound) */
    Chip8Scancode keyboard;
    
    /** Alternate keyboard scancode (-1 if unbound) */
    Chip8Scancode keyboard_alt;
    
    /** Gamepad button (-1 if unbound) */
    Chip8GamepadButton gamepad_button;
} Chip8KeyBinding;

/**
 * @brief Input settings including key mappings and gamepad config
 */
typedef struct Chip8InputSettings {
    /** Key bindings for each CHIP-8 key (0-F) */
    Chip8KeyBinding bindings[16];
    
    /** Enable gamepad support */
    bool gamepad_enabled;
    
    /** Active gamepad index (0-3) */
    int active_gamepad;
    
    /** Gamepad deadzone for analog sticks (0.0 - 1.0) */
    float analog_deadzone;
    
    /** Use left stick for directional input (keys 2,4,6,8) */
    bool use_left_stick;
    
    /** Use D-pad for directional input */
    bool use_dpad;
    
    /** Vibration feedback on key press */
    bool vibration_enabled;
    
    /** Vibration intensity (0.0 - 1.0) */
    float vibration_intensity;
} Chip8InputSettings;

/**
 * @brief Gamepad info for display
 */
typedef struct Chip8GamepadInfo {
    bool connected;
    char name[128];
    int player_index;
    bool has_rumble;
} Chip8GamepadInfo;

/**
 * @brief CHIP-8 quirk flags for compatibility
 * 
 * Different CHIP-8 implementations have subtle behavioral differences.
 * These flags control which behaviors are enabled.
 */
typedef struct Chip8Quirks {
    /** VF is reset to 0 after AND, OR, XOR (original COSMAC VIP behavior) */
    bool vf_reset;
    
    /** Shift instructions use VY as source (8XY6, 8XYE) */
    bool shift_uses_vy;
    
    /** Load/Store (FX55, FX65) increment I register */
    bool memory_increment_i;
    
    /** Sprites wrap around screen edges (vs clip) */
    bool sprite_wrap;
    
    /** BNNN jumps to XNN + VX (not V0) on SUPER-CHIP */
    bool jump_uses_vx;
    
    /** Display waits for VBLANK before drawing (60Hz sync) */
    bool display_wait;
} Chip8Quirks;

/* ============================================================================
 * Settings Structure
 * ========================================================================== */

/**
 * @brief Graphics settings
 */
typedef struct Chip8GraphicsSettings {
    /** Window size preset */
    Chip8WindowSize window_size;
    
    /** Window scale factor (1-20, default 10) */
    int scale;
    
    /** Fullscreen mode enabled */
    bool fullscreen;
    
    /** Current color theme */
    Chip8ColorTheme theme;
    
    /** Custom foreground color (when theme == CUSTOM) */
    Chip8Color custom_fg;
    
    /** Custom background color (when theme == CUSTOM) */
    Chip8Color custom_bg;
    
    /** Show pixel grid overlay */
    bool pixel_grid;
    
    /** Enable CRT scanline effect */
    bool crt_effect;
    
    /** Scanline intensity (0.0 - 1.0) */
    float scanline_intensity;
    
    /** Enable screen curvature effect */
    bool screen_curve;
} Chip8GraphicsSettings;

/**
 * @brief Audio settings
 */
typedef struct Chip8AudioSettings {
    /** Master volume (0.0 - 1.0) */
    float volume;
    
    /** Beep frequency in Hz (220 - 880, default 440) */
    int frequency;
    
    /** Waveform type */
    Chip8Waveform waveform;
    
    /** Audio muted */
    bool muted;
} Chip8AudioSettings;

/**
 * @brief Gameplay settings
 */
typedef struct Chip8GameplaySettings {
    /** CPU frequency in Hz (100 - 2000, default 700) */
    int cpu_freq_hz;
    
    /** Key repeat delay in milliseconds (100 - 1000) */
    int key_repeat_delay_ms;
    
    /** Key repeat rate in milliseconds (50 - 500) */
    int key_repeat_rate_ms;
    
    /** Quirk settings for compatibility */
    Chip8Quirks quirks;
} Chip8GameplaySettings;

/**
 * @brief Complete runtime settings
 */
typedef struct Chip8Settings {
    Chip8GraphicsSettings graphics;
    Chip8AudioSettings audio;
    Chip8GameplaySettings gameplay;
    Chip8InputSettings input;
} Chip8Settings;

/* ============================================================================
 * Settings Functions
 * ========================================================================== */

/**
 * @brief Get default settings
 * 
 * @return Settings initialized with default values
 */
Chip8Settings chip8_settings_default(void);

/**
 * @brief Load settings from a config file
 * 
 * @param settings Settings structure to populate
 * @param path Path to config file
 * @return true on success, false on failure
 */
bool chip8_settings_load(Chip8Settings* settings, const char* path);

/**
 * @brief Save settings to a config file
 * 
 * @param settings Settings to save
 * @param path Path to config file
 * @return true on success, false on failure
 */
bool chip8_settings_save(const Chip8Settings* settings, const char* path);

/**
 * @brief Get the default config file path
 * 
 * Returns a path to the user's config file, typically in their home directory.
 * The path is: ~/.chip8recompiled/settings.ini
 * 
 * @return Static buffer containing the path, or NULL on error
 */
const char* chip8_settings_get_default_path(void);

/**
 * @brief Get the config file path for a specific ROM
 * 
 * Returns a path to the ROM-specific config file.
 * The path is: ~/.chip8recompiled/games/<rom_name>.ini
 * 
 * @param rom_name Name of the ROM (will be sanitized for filesystem)
 * @return Static buffer containing the path, or NULL on error
 */
const char* chip8_settings_get_rom_path(const char* rom_name);

/**
 * @brief Get the window size name as string
 * 
 * @param size Window size preset
 * @return Window size name
 */
const char* chip8_get_window_size_name(Chip8WindowSize size);

/**
 * @brief Get the scale value for a window size preset
 * 
 * @param size Window size preset
 * @return Scale value (1-20)
 */
int chip8_get_window_size_scale(Chip8WindowSize size);

/**
 * @brief Get theme colors for a given theme
 * 
 * @param theme Theme to get colors for
 * @return Theme colors
 */
Chip8ThemeColors chip8_get_theme_colors(Chip8ColorTheme theme);

/**
 * @brief Get theme name as string
 * 
 * @param theme Theme to get name for
 * @return Theme name
 */
const char* chip8_get_theme_name(Chip8ColorTheme theme);

/**
 * @brief Get waveform name as string
 * 
 * @param waveform Waveform to get name for
 * @return Waveform name
 */
const char* chip8_get_waveform_name(Chip8Waveform waveform);

/**
 * @brief Get default input bindings
 * 
 * Returns the standard keyboard mapping for CHIP-8.
 * 
 * @param input Input settings structure to initialize
 */
void chip8_input_settings_default(Chip8InputSettings* input);

/**
 * @brief Get the name of a keyboard scancode
 * 
 * @param scancode The scancode to get the name for
 * @return Human-readable key name
 */
const char* chip8_get_scancode_name(Chip8Scancode scancode);

/**
 * @brief Get the name of a gamepad button
 * 
 * @param button The gamepad button
 * @return Human-readable button name
 */
const char* chip8_get_gamepad_button_name(Chip8GamepadButton button);

/**
 * @brief Get the CHIP-8 key label (0-9, A-F)
 * 
 * @param key CHIP-8 key index (0-15)
 * @return Single character label
 */
const char* chip8_get_key_label(int key);

#ifdef __cplusplus
}
#endif

#endif /* CHIP8RT_SETTINGS_H */
