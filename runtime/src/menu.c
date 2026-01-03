/**
 * @file menu.c
 * @brief In-game pause menu implementation
 */

#include "chip8rt/menu.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Menu Item Definitions
 * ========================================================================== */

/* Multi-ROM mode flag */
static bool g_multi_rom_mode = false;

/* Main pause menu items (regular mode) */
static const char* PAUSE_MENU_ITEMS[] = {
    "Resume",
    "Graphics",
    "Audio", 
    "Gameplay",
    "Quirks",
    "Controls",
    "Reset Game",
    "Quit"
};
#define PAUSE_MENU_COUNT 8

/* Main pause menu items (multi-ROM mode) */
static const char* PAUSE_MENU_ITEMS_MULTI[] = {
    "Resume",
    "Graphics",
    "Audio", 
    "Gameplay",
    "Quirks",
    "Controls",
    "Reset Game",
    "Back to Menu",
    "Quit"
};
#define PAUSE_MENU_COUNT_MULTI 9

/* Graphics settings items */
static const char* GRAPHICS_MENU_ITEMS[] = {
    "Window Size",
    "Fullscreen",
    "Color Theme",
    "Pixel Grid",
    "CRT Effect",
    "Scanlines",
    "Back"
};
#define GRAPHICS_MENU_COUNT 7

/* Audio settings items */
static const char* AUDIO_MENU_ITEMS[] = {
    "Volume",
    "Frequency",
    "Waveform",
    "Muted",
    "Back"
};
#define AUDIO_MENU_COUNT 5

/* Gameplay settings items */
static const char* GAMEPLAY_MENU_ITEMS[] = {
    "CPU Speed",
    "Key Repeat Delay",
    "Key Repeat Rate",
    "Back"
};
#define GAMEPLAY_MENU_COUNT 4

/* Quirks settings items */
static const char* QUIRKS_MENU_ITEMS[] = {
    "VF Reset",
    "Shift uses VY",
    "Memory incr I",
    "Sprite Wrap",
    "Jump uses VX",
    "Display Wait",
    "Back"
};
#define QUIRKS_MENU_COUNT 7

/* Confirm dialog items */
static const char* CONFIRM_ITEMS[] = {
    "Yes",
    "No"
};
#define CONFIRM_COUNT 2

/* ============================================================================
 * Static value buffers for display
 * ========================================================================== */

static char g_value_buffer[64];

/* ============================================================================
 * Menu Functions
 * ========================================================================== */

void chip8_menu_set_multi_rom_mode(bool enabled) {
    g_multi_rom_mode = enabled;
}

bool chip8_menu_is_multi_rom_mode(void) {
    return g_multi_rom_mode;
}

void chip8_menu_init(Chip8MenuState* menu, const Chip8Settings* settings) {
    memset(menu, 0, sizeof(Chip8MenuState));
    menu->screen = CHIP8_MENU_NONE;
    menu->selected = 0;
    menu->item_count = 0;
    menu->paused = false;
    menu->reset_requested = false;
    menu->quit_requested = false;
    menu->menu_requested = false;
    menu->settings_dirty = false;
    
    if (settings) {
        menu->settings = *settings;
    } else {
        menu->settings = chip8_settings_default();
    }
}

void chip8_menu_open(Chip8MenuState* menu) {
    menu->screen = CHIP8_MENU_PAUSE;
    menu->selected = 0;
    menu->item_count = g_multi_rom_mode ? PAUSE_MENU_COUNT_MULTI : PAUSE_MENU_COUNT;
    menu->paused = true;
}

void chip8_menu_close(Chip8MenuState* menu) {
    menu->screen = CHIP8_MENU_NONE;
    menu->selected = 0;
    menu->item_count = 0;
    menu->paused = false;
}

bool chip8_menu_is_open(const Chip8MenuState* menu) {
    return menu->screen != CHIP8_MENU_NONE;
}

static void enter_submenu(Chip8MenuState* menu, Chip8MenuScreen screen, int item_count) {
    menu->screen = screen;
    menu->selected = 0;
    menu->item_count = item_count;
}

static void go_back(Chip8MenuState* menu) {
    switch (menu->screen) {
        case CHIP8_MENU_GRAPHICS:
        case CHIP8_MENU_AUDIO:
        case CHIP8_MENU_GAMEPLAY:
        case CHIP8_MENU_QUIRKS:
        case CHIP8_MENU_CONTROLS:
            menu->screen = CHIP8_MENU_PAUSE;
            menu->selected = 0;
            menu->item_count = PAUSE_MENU_COUNT;
            break;
        case CHIP8_MENU_CONFIRM_QUIT:
        case CHIP8_MENU_CONFIRM_RESET:
            menu->screen = CHIP8_MENU_PAUSE;
            menu->selected = 0;
            menu->item_count = PAUSE_MENU_COUNT;
            break;
        case CHIP8_MENU_PAUSE:
            chip8_menu_close(menu);
            break;
        default:
            break;
    }
}

static void adjust_value(Chip8MenuState* menu, int delta) {
    Chip8Settings* s = &menu->settings;
    
    switch (menu->screen) {
        case CHIP8_MENU_GRAPHICS:
            switch (menu->selected) {
                case 0: /* Window Size */
                    s->graphics.window_size = (s->graphics.window_size + delta + CHIP8_WINDOW_COUNT) % CHIP8_WINDOW_COUNT;
                    /* Update scale to match window size preset (unless custom) */
                    if (s->graphics.window_size != CHIP8_WINDOW_CUSTOM) {
                        s->graphics.scale = chip8_get_window_size_scale(s->graphics.window_size);
                    }
                    menu->settings_dirty = true;
                    break;
                case 1: /* Fullscreen */
                    s->graphics.fullscreen = !s->graphics.fullscreen;
                    menu->settings_dirty = true;
                    break;
                case 2: /* Theme */
                    s->graphics.theme = (s->graphics.theme + delta + CHIP8_THEME_COUNT) % CHIP8_THEME_COUNT;
                    menu->settings_dirty = true;
                    break;
                case 3: /* Pixel Grid */
                    s->graphics.pixel_grid = !s->graphics.pixel_grid;
                    menu->settings_dirty = true;
                    break;
                case 4: /* CRT Effect */
                    s->graphics.crt_effect = !s->graphics.crt_effect;
                    menu->settings_dirty = true;
                    break;
                case 5: /* Scanlines */
                    s->graphics.scanline_intensity += delta * 0.1f;
                    if (s->graphics.scanline_intensity < 0.0f) s->graphics.scanline_intensity = 0.0f;
                    if (s->graphics.scanline_intensity > 1.0f) s->graphics.scanline_intensity = 1.0f;
                    menu->settings_dirty = true;
                    break;
            }
            break;
            
        case CHIP8_MENU_AUDIO:
            switch (menu->selected) {
                case 0: /* Volume */
                    s->audio.volume += delta * 0.1f;
                    if (s->audio.volume < 0.0f) s->audio.volume = 0.0f;
                    if (s->audio.volume > 1.0f) s->audio.volume = 1.0f;
                    menu->settings_dirty = true;
                    break;
                case 1: /* Frequency */
                    s->audio.frequency += delta * 20;
                    if (s->audio.frequency < 220) s->audio.frequency = 220;
                    if (s->audio.frequency > 880) s->audio.frequency = 880;
                    menu->settings_dirty = true;
                    break;
                case 2: /* Waveform */
                    s->audio.waveform = (s->audio.waveform + delta + CHIP8_WAVE_COUNT) % CHIP8_WAVE_COUNT;
                    menu->settings_dirty = true;
                    break;
                case 3: /* Muted */
                    s->audio.muted = !s->audio.muted;
                    menu->settings_dirty = true;
                    break;
            }
            break;
            
        case CHIP8_MENU_GAMEPLAY:
            switch (menu->selected) {
                case 0: /* CPU Speed */
                    s->gameplay.cpu_freq_hz += delta * 50;
                    if (s->gameplay.cpu_freq_hz < 100) s->gameplay.cpu_freq_hz = 100;
                    if (s->gameplay.cpu_freq_hz > 2000) s->gameplay.cpu_freq_hz = 2000;
                    menu->settings_dirty = true;
                    break;
                case 1: /* Key Repeat Delay */
                    s->gameplay.key_repeat_delay_ms += delta * 50;
                    if (s->gameplay.key_repeat_delay_ms < 100) s->gameplay.key_repeat_delay_ms = 100;
                    if (s->gameplay.key_repeat_delay_ms > 1000) s->gameplay.key_repeat_delay_ms = 1000;
                    menu->settings_dirty = true;
                    break;
                case 2: /* Key Repeat Rate */
                    s->gameplay.key_repeat_rate_ms += delta * 25;
                    if (s->gameplay.key_repeat_rate_ms < 50) s->gameplay.key_repeat_rate_ms = 50;
                    if (s->gameplay.key_repeat_rate_ms > 500) s->gameplay.key_repeat_rate_ms = 500;
                    menu->settings_dirty = true;
                    break;
            }
            break;
            
        case CHIP8_MENU_QUIRKS:
            if (menu->selected < 6) {
                bool* quirk = NULL;
                switch (menu->selected) {
                    case 0: quirk = &s->gameplay.quirks.vf_reset; break;
                    case 1: quirk = &s->gameplay.quirks.shift_uses_vy; break;
                    case 2: quirk = &s->gameplay.quirks.memory_increment_i; break;
                    case 3: quirk = &s->gameplay.quirks.sprite_wrap; break;
                    case 4: quirk = &s->gameplay.quirks.jump_uses_vx; break;
                    case 5: quirk = &s->gameplay.quirks.display_wait; break;
                }
                if (quirk) {
                    *quirk = !*quirk;
                    menu->settings_dirty = true;
                }
            }
            break;
            
        default:
            break;
    }
}

static void select_item(Chip8MenuState* menu) {
    switch (menu->screen) {
        case CHIP8_MENU_PAUSE:
            switch (menu->selected) {
                case 0: /* Resume */
                    chip8_menu_close(menu);
                    break;
                case 1: /* Graphics */
                    enter_submenu(menu, CHIP8_MENU_GRAPHICS, GRAPHICS_MENU_COUNT);
                    break;
                case 2: /* Audio */
                    enter_submenu(menu, CHIP8_MENU_AUDIO, AUDIO_MENU_COUNT);
                    break;
                case 3: /* Gameplay */
                    enter_submenu(menu, CHIP8_MENU_GAMEPLAY, GAMEPLAY_MENU_COUNT);
                    break;
                case 4: /* Quirks */
                    enter_submenu(menu, CHIP8_MENU_QUIRKS, QUIRKS_MENU_COUNT);
                    break;
                case 5: /* Controls */
                    enter_submenu(menu, CHIP8_MENU_CONTROLS, 1); /* Just "Back" */
                    break;
                case 6: /* Reset */
                    enter_submenu(menu, CHIP8_MENU_CONFIRM_RESET, CONFIRM_COUNT);
                    break;
                case 7: /* Quit or Back to Menu */
                    if (g_multi_rom_mode) {
                        /* Back to Menu */
                        enter_submenu(menu, CHIP8_MENU_CONFIRM_MENU, CONFIRM_COUNT);
                    } else {
                        /* Quit */
                        enter_submenu(menu, CHIP8_MENU_CONFIRM_QUIT, CONFIRM_COUNT);
                    }
                    break;
                case 8: /* Quit (multi-ROM mode only) */
                    if (g_multi_rom_mode) {
                        enter_submenu(menu, CHIP8_MENU_CONFIRM_QUIT, CONFIRM_COUNT);
                    }
                    break;
            }
            break;
            
        case CHIP8_MENU_GRAPHICS:
            if (menu->selected == GRAPHICS_MENU_COUNT - 1) {
                go_back(menu);
            } else {
                /* Toggle/adjust on select */
                adjust_value(menu, 1);
            }
            break;
            
        case CHIP8_MENU_AUDIO:
            if (menu->selected == AUDIO_MENU_COUNT - 1) {
                go_back(menu);
            } else {
                adjust_value(menu, 1);
            }
            break;
            
        case CHIP8_MENU_GAMEPLAY:
            if (menu->selected == GAMEPLAY_MENU_COUNT - 1) {
                go_back(menu);
            } else {
                adjust_value(menu, 1);
            }
            break;
            
        case CHIP8_MENU_QUIRKS:
            if (menu->selected == QUIRKS_MENU_COUNT - 1) {
                go_back(menu);
            } else {
                adjust_value(menu, 1);
            }
            break;
            
        case CHIP8_MENU_CONTROLS:
            go_back(menu);
            break;
            
        case CHIP8_MENU_CONFIRM_QUIT:
            if (menu->selected == 0) { /* Yes */
                menu->quit_requested = true;
            }
            go_back(menu);
            break;
            
        case CHIP8_MENU_CONFIRM_RESET:
            if (menu->selected == 0) { /* Yes */
                menu->reset_requested = true;
                chip8_menu_close(menu);
            } else {
                go_back(menu);
            }
            break;
            
        case CHIP8_MENU_CONFIRM_MENU:
            if (menu->selected == 0) { /* Yes */
                menu->menu_requested = true;
                chip8_menu_close(menu);
            } else {
                go_back(menu);
            }
            break;
            
        default:
            break;
    }
}

void chip8_menu_navigate(Chip8MenuState* menu, Chip8MenuNav nav) {
    switch (nav) {
        case CHIP8_NAV_UP:
            if (menu->selected > 0) {
                menu->selected--;
            }
            break;
            
        case CHIP8_NAV_DOWN:
            if (menu->selected < menu->item_count - 1) {
                menu->selected++;
            }
            break;
            
        case CHIP8_NAV_LEFT:
            adjust_value(menu, -1);
            break;
            
        case CHIP8_NAV_RIGHT:
            adjust_value(menu, 1);
            break;
            
        case CHIP8_NAV_SELECT:
            select_item(menu);
            break;
            
        case CHIP8_NAV_BACK:
            go_back(menu);
            break;
            
        default:
            break;
    }
}

const char* chip8_menu_get_title(const Chip8MenuState* menu) {
    switch (menu->screen) {
        case CHIP8_MENU_PAUSE:        return "PAUSED";
        case CHIP8_MENU_GRAPHICS:     return "Graphics";
        case CHIP8_MENU_AUDIO:        return "Audio";
        case CHIP8_MENU_GAMEPLAY:     return "Gameplay";
        case CHIP8_MENU_QUIRKS:       return "Quirks";
        case CHIP8_MENU_CONTROLS:     return "Controls";
        case CHIP8_MENU_CONFIRM_QUIT: return "Quit Game?";
        case CHIP8_MENU_CONFIRM_RESET:return "Reset Game?";
        case CHIP8_MENU_CONFIRM_MENU: return "Return to Menu?";
        default:                       return "";
    }
}

const char* chip8_menu_get_item_label(const Chip8MenuState* menu, int index) {
    if (index < 0 || index >= menu->item_count) return NULL;
    
    switch (menu->screen) {
        case CHIP8_MENU_PAUSE:
            return g_multi_rom_mode ? PAUSE_MENU_ITEMS_MULTI[index] : PAUSE_MENU_ITEMS[index];
        case CHIP8_MENU_GRAPHICS:
            return GRAPHICS_MENU_ITEMS[index];
        case CHIP8_MENU_AUDIO:
            return AUDIO_MENU_ITEMS[index];
        case CHIP8_MENU_GAMEPLAY:
            return GAMEPLAY_MENU_ITEMS[index];
        case CHIP8_MENU_QUIRKS:
            return QUIRKS_MENU_ITEMS[index];
        case CHIP8_MENU_CONTROLS:
            return "Back";
        case CHIP8_MENU_CONFIRM_QUIT:
        case CHIP8_MENU_CONFIRM_RESET:
        case CHIP8_MENU_CONFIRM_MENU:
            return CONFIRM_ITEMS[index];
        default:
            return NULL;
    }
}

const char* chip8_menu_get_item_value(const Chip8MenuState* menu, int index) {
    if (index < 0 || index >= menu->item_count) return NULL;
    
    const Chip8Settings* s = &menu->settings;
    
    switch (menu->screen) {
        case CHIP8_MENU_GRAPHICS:
            switch (index) {
                case 0: /* Window Size */
                    return chip8_get_window_size_name(s->graphics.window_size);
                case 1: /* Fullscreen */
                    return s->graphics.fullscreen ? "On" : "Off";
                case 2: /* Theme */
                    return chip8_get_theme_name(s->graphics.theme);
                case 3: /* Pixel Grid */
                    return s->graphics.pixel_grid ? "On" : "Off";
                case 4: /* CRT Effect */
                    return s->graphics.crt_effect ? "On" : "Off";
                case 5: /* Scanlines */
                    snprintf(g_value_buffer, sizeof(g_value_buffer), "%.0f%%", s->graphics.scanline_intensity * 100);
                    return g_value_buffer;
            }
            break;
            
        case CHIP8_MENU_AUDIO:
            switch (index) {
                case 0: /* Volume */
                    snprintf(g_value_buffer, sizeof(g_value_buffer), "%.0f%%", s->audio.volume * 100);
                    return g_value_buffer;
                case 1: /* Frequency */
                    snprintf(g_value_buffer, sizeof(g_value_buffer), "%d Hz", s->audio.frequency);
                    return g_value_buffer;
                case 2: /* Waveform */
                    return chip8_get_waveform_name(s->audio.waveform);
                case 3: /* Muted */
                    return s->audio.muted ? "Yes" : "No";
            }
            break;
            
        case CHIP8_MENU_GAMEPLAY:
            switch (index) {
                case 0: /* CPU Speed */
                    snprintf(g_value_buffer, sizeof(g_value_buffer), "%d Hz", s->gameplay.cpu_freq_hz);
                    return g_value_buffer;
                case 1: /* Key Repeat Delay */
                    snprintf(g_value_buffer, sizeof(g_value_buffer), "%d ms", s->gameplay.key_repeat_delay_ms);
                    return g_value_buffer;
                case 2: /* Key Repeat Rate */
                    snprintf(g_value_buffer, sizeof(g_value_buffer), "%d ms", s->gameplay.key_repeat_rate_ms);
                    return g_value_buffer;
            }
            break;
            
        case CHIP8_MENU_QUIRKS:
            if (index < 6) {
                bool value = false;
                switch (index) {
                    case 0: value = s->gameplay.quirks.vf_reset; break;
                    case 1: value = s->gameplay.quirks.shift_uses_vy; break;
                    case 2: value = s->gameplay.quirks.memory_increment_i; break;
                    case 3: value = s->gameplay.quirks.sprite_wrap; break;
                    case 4: value = s->gameplay.quirks.jump_uses_vx; break;
                    case 5: value = s->gameplay.quirks.display_wait; break;
                }
                return value ? "On" : "Off";
            }
            break;
            
        default:
            break;
    }
    
    return NULL;
}

bool chip8_menu_is_item_selected(const Chip8MenuState* menu, int index) {
    return menu->selected == index;
}

void chip8_menu_apply_settings(Chip8MenuState* menu, Chip8Settings* settings) {
    if (menu->settings_dirty) {
        *settings = menu->settings;
        menu->settings_dirty = false;
    }
}
