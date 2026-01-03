/**
 * @file menu.h
 * @brief In-game pause menu and settings UI
 * 
 * Simple overlay menu system for runtime settings.
 */

#ifndef CHIP8RT_MENU_H
#define CHIP8RT_MENU_H

#include <stdbool.h>
#include "context.h"
#include "settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Menu State
 * ========================================================================== */

/**
 * @brief Menu screen types
 */
typedef enum Chip8MenuScreen {
    CHIP8_MENU_NONE,           /* No menu, game running */
    CHIP8_MENU_PAUSE,          /* Main pause menu */
    CHIP8_MENU_GRAPHICS,       /* Graphics settings */
    CHIP8_MENU_AUDIO,          /* Audio settings */
    CHIP8_MENU_GAMEPLAY,       /* Gameplay settings */
    CHIP8_MENU_QUIRKS,         /* Quirk toggles */
    CHIP8_MENU_CONTROLS,       /* Control reference */
    CHIP8_MENU_CONFIRM_QUIT,   /* Quit confirmation */
    CHIP8_MENU_CONFIRM_RESET,  /* Reset confirmation */
    CHIP8_MENU_CONFIRM_MENU    /* Return to menu confirmation */
} Chip8MenuScreen;

/**
 * @brief Menu navigation direction
 */
typedef enum Chip8MenuNav {
    CHIP8_NAV_NONE,
    CHIP8_NAV_UP,
    CHIP8_NAV_DOWN,
    CHIP8_NAV_LEFT,
    CHIP8_NAV_RIGHT,
    CHIP8_NAV_SELECT,
    CHIP8_NAV_BACK
} Chip8MenuNav;

/**
 * @brief Menu state structure
 */
typedef struct Chip8MenuState {
    /** Current menu screen */
    Chip8MenuScreen screen;
    
    /** Selected menu item index */
    int selected;
    
    /** Number of items in current menu */
    int item_count;
    
    /** Settings being edited (copy for cancel support) */
    Chip8Settings settings;
    
    /** Whether settings have been modified */
    bool settings_dirty;
    
    /** Game paused flag */
    bool paused;
    
    /** Request to reset the game */
    bool reset_requested;
    
    /** Request to quit */
    bool quit_requested;
    
    /** Request to return to ROM menu (multi-ROM launcher only) */
    bool menu_requested;
} Chip8MenuState;

/* ============================================================================
 * Menu Functions
 * ========================================================================== */

/**
 * @brief Initialize menu state
 * 
 * @param menu Menu state to initialize
 * @param settings Current settings to copy
 */
void chip8_menu_init(Chip8MenuState* menu, const Chip8Settings* settings);

/**
 * @brief Set multi-ROM mode for menu
 * 
 * When enabled, adds "Back to Menu" option to pause menu.
 * 
 * @param enabled true to enable multi-ROM mode
 */
void chip8_menu_set_multi_rom_mode(bool enabled);

/**
 * @brief Check if multi-ROM mode is enabled
 * 
 * @return true if multi-ROM mode is active
 */
bool chip8_menu_is_multi_rom_mode(void);

/**
 * @brief Open the pause menu
 * 
 * @param menu Menu state
 */
void chip8_menu_open(Chip8MenuState* menu);

/**
 * @brief Close the menu and resume game
 * 
 * @param menu Menu state
 */
void chip8_menu_close(Chip8MenuState* menu);

/**
 * @brief Check if menu is open
 * 
 * @param menu Menu state
 * @return true if menu is displayed
 */
bool chip8_menu_is_open(const Chip8MenuState* menu);

/**
 * @brief Handle menu navigation input
 * 
 * @param menu Menu state
 * @param nav Navigation direction
 */
void chip8_menu_navigate(Chip8MenuState* menu, Chip8MenuNav nav);

/**
 * @brief Get current menu title
 * 
 * @param menu Menu state
 * @return Menu title string
 */
const char* chip8_menu_get_title(const Chip8MenuState* menu);

/**
 * @brief Get menu item label
 * 
 * @param menu Menu state
 * @param index Item index
 * @return Item label string, or NULL if invalid index
 */
const char* chip8_menu_get_item_label(const Chip8MenuState* menu, int index);

/**
 * @brief Get menu item value (for settings)
 * 
 * @param menu Menu state
 * @param index Item index
 * @return Item value string, or NULL if not applicable
 */
const char* chip8_menu_get_item_value(const Chip8MenuState* menu, int index);

/**
 * @brief Check if menu item is selected
 * 
 * @param menu Menu state
 * @param index Item index
 * @return true if item is currently selected
 */
bool chip8_menu_is_item_selected(const Chip8MenuState* menu, int index);

/**
 * @brief Apply current settings
 * 
 * @param menu Menu state
 * @param settings Settings to update with menu values
 */
void chip8_menu_apply_settings(Chip8MenuState* menu, Chip8Settings* settings);

#ifdef __cplusplus
}
#endif

#endif /* CHIP8RT_MENU_H */
