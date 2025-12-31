/**
 * @file imgui_overlay.h
 * @brief ImGui overlay for debug info, FPS counter, and settings
 */

#ifndef CHIP8RT_IMGUI_OVERLAY_H
#define CHIP8RT_IMGUI_OVERLAY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct SDL_Window;
struct SDL_Renderer;
struct Chip8Context;
struct Chip8Settings;
union SDL_Event;

/**
 * @brief Overlay display state
 */
typedef struct Chip8OverlayState {
    bool show_fps;           /* Show FPS counter */
    bool show_debug;         /* Show debug overlay (registers, memory) */
    bool show_settings;      /* Show settings window */
    bool show_rom_info;      /* Show ROM information */
    bool show_demo;          /* Show ImGui demo window (for development) */
    
    /* Settings change tracking */
    bool settings_changed;   /* Set when ImGui modifies settings */
    
    /* FPS tracking */
    float fps;
    float frame_time_ms;
    int frame_count;
    uint64_t last_fps_time;
    
    /* Performance history */
    float fps_history[120];
    int fps_history_idx;
} Chip8OverlayState;

/**
 * @brief Initialize the ImGui overlay system
 * 
 * @param window SDL window
 * @param renderer SDL renderer
 * @return true on success
 */
bool chip8_overlay_init(struct SDL_Window* window, struct SDL_Renderer* renderer);

/**
 * @brief Shutdown the ImGui overlay system
 */
void chip8_overlay_shutdown(void);

/**
 * @brief Process SDL event for ImGui
 * 
 * @param event SDL event
 * @return true if event was consumed by ImGui
 */
bool chip8_overlay_process_event(union SDL_Event* event);

/**
 * @brief Begin a new ImGui frame
 */
void chip8_overlay_new_frame(void);

/**
 * @brief Render all overlay windows
 * 
 * @param ctx CHIP-8 context
 * @param state Overlay state
 * @param settings Current settings
 */
void chip8_overlay_render(struct Chip8Context* ctx, 
                          Chip8OverlayState* state,
                          struct Chip8Settings* settings);

/**
 * @brief Finish rendering and present ImGui
 * 
 * @param renderer SDL renderer
 */
void chip8_overlay_present(struct SDL_Renderer* renderer);

/**
 * @brief Update FPS counter
 * 
 * @param state Overlay state
 * @param current_time_us Current time in microseconds
 */
void chip8_overlay_update_fps(Chip8OverlayState* state, uint64_t current_time_us);

/**
 * @brief Check if ImGui wants to capture keyboard input
 * 
 * @return true if ImGui is using keyboard
 */
bool chip8_overlay_wants_keyboard(void);

/**
 * @brief Check if ImGui wants to capture mouse input
 * 
 * @return true if ImGui is using mouse
 */
bool chip8_overlay_wants_mouse(void);

/**
 * @brief Toggle debug overlay visibility
 * 
 * @param state Overlay state
 */
void chip8_overlay_toggle_debug(Chip8OverlayState* state);

/**
 * @brief Toggle FPS display
 * 
 * @param state Overlay state
 */
void chip8_overlay_toggle_fps(Chip8OverlayState* state);

#ifdef __cplusplus
}
#endif

#endif /* CHIP8RT_IMGUI_OVERLAY_H */
