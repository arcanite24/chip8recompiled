/**
 * @file rom_selector.cpp
 * @brief ROM selection menu implementation using ImGui
 */

#include "chip8rt/rom_catalog.h"
#include "chip8rt/runtime.h"
#include "chip8rt/platform.h"
#include "chip8rt/settings.h"
#include "chip8rt/menu.h"

#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cmath>

/* Forward declare from platform_sdl.c */
extern "C" {
    extern SDL_Window* g_sdl_window;
    extern SDL_Renderer* g_sdl_renderer;
    extern bool chip8_overlay_init(SDL_Window* window, SDL_Renderer* renderer);
    extern void chip8_overlay_shutdown(void);
    extern void chip8_overlay_new_frame(void);
}

/* ============================================================================
 * ROM Selection State
 * ========================================================================== */

static int g_selected_rom = 0;
static bool g_return_to_menu = false;
static bool g_should_quit = false;
static bool g_launch_selected = false;
static float g_animation_time = 0.0f;

/* ============================================================================
 * Custom Styling
 * ========================================================================== */

static void setup_rom_selector_style() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    /* Larger, more readable sizes */
    style.WindowPadding = ImVec2(20, 20);
    style.FramePadding = ImVec2(12, 8);
    style.ItemSpacing = ImVec2(10, 10);
    style.ItemInnerSpacing = ImVec2(10, 6);
    style.ScrollbarSize = 16;
    style.ScrollbarRounding = 8;
    style.GrabMinSize = 14;
    style.WindowRounding = 0;
    style.FrameRounding = 6;
    style.GrabRounding = 4;
    
    /* Retro-inspired color scheme */
    ImVec4* colors = style.Colors;
    
    /* Dark purple/blue background */
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.06f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.04f, 0.10f, 1.00f);
    
    /* Green accent (classic retro) */
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.45f, 0.25f, 0.90f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.60f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.70f, 0.30f, 1.00f);
    
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.35f, 0.20f, 0.90f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.55f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.65f, 0.35f, 1.00f);
    
    /* Scrollbar */
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.03f, 0.08f, 0.90f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.40f, 0.25f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.55f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.65f, 0.40f, 1.00f);
    
    /* Text colors */
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.95f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.55f, 0.50f, 1.00f);
    
    /* Border */
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.50f, 0.30f, 0.50f);
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.50f, 0.30f, 0.50f);
}

/* ============================================================================
 * ROM Selection Menu Rendering
 * ========================================================================== */

static int render_rom_selection_menu(const RomEntry* catalog, size_t count) {
    ImGuiIO& io = ImGui::GetIO();
    g_animation_time += io.DeltaTime;
    
    /* Setup custom style */
    setup_rom_selector_style();
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    ImGui::Begin("ROM Selector", nullptr, window_flags);
    
    float window_width = ImGui::GetWindowWidth();
    float window_height = ImGui::GetWindowHeight();
    
    /* ===== Header Section ===== */
    ImGui::Spacing();
    
    /* Animated title with glow effect */
    float glow = 0.5f + 0.5f * sinf(g_animation_time * 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f + 0.3f * glow, 0.9f, 0.5f + 0.2f * glow, 1.0f));
    
    ImGui::SetWindowFontScale(2.5f);
    const char* title = "CHIP-8 Collection";
    float title_width = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((window_width - title_width) * 0.5f);
    ImGui::Text("%s", title);
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::PopStyleColor();
    
    /* Subtitle */
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.65f, 0.6f, 1.0f));
    ImGui::SetWindowFontScale(1.2f);
    const char* subtitle = "Select a game to play";
    float subtitle_width = ImGui::CalcTextSize(subtitle).x;
    ImGui::SetCursorPosX((window_width - subtitle_width) * 0.5f);
    ImGui::Text("%s", subtitle);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    /* Decorative separator */
    float sep_width = window_width * 0.6f;
    ImGui::SetCursorPosX((window_width - sep_width) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.3f, 0.6f, 0.35f, 0.8f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    /* ===== ROM Grid/List ===== */
    float list_height = window_height - 220;  /* Leave room for header and footer */
    
    /* Center the list */
    float list_width = fminf(window_width - 80, 800.0f);
    ImGui::SetCursorPosX((window_width - list_width) * 0.5f);
    
    ImGui::BeginChild("ROMList", ImVec2(list_width, list_height), true);
    
    int selected = -1;
    
    for (size_t i = 0; i < count; i++) {
        ImGui::PushID((int)i);
        
        bool is_selected = (g_selected_rom == (int)i);
        
        /* Selection highlight */
        if (is_selected) {
            float pulse = 0.9f + 0.1f * sinf(g_animation_time * 4.0f);
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f * pulse, 0.50f * pulse, 0.25f * pulse, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.60f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.30f, 0.70f, 0.35f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.10f, 0.12f, 0.14f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.15f, 0.40f, 0.20f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.20f, 0.50f, 0.25f, 1.0f));
        }
        
        /* Format: "01  Game Title [Author]" */
        char label[256];
        snprintf(label, sizeof(label), "%02d    %s", (int)(i + 1), catalog[i].title);
        
        if (ImGui::Selectable(label, is_selected, ImGuiSelectableFlags_None, ImVec2(0, 40))) {
            selected = (int)i;
        }
        
        /* Update selection on click */
        if (ImGui::IsItemClicked()) {
            g_selected_rom = (int)i;
        }
        
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }
    
    /* Auto-scroll to selected item */
    if (g_selected_rom >= 0 && g_selected_rom < (int)count) {
        float item_height = 40.0f + ImGui::GetStyle().ItemSpacing.y;
        float item_pos = g_selected_rom * item_height;
        float scroll_y = ImGui::GetScrollY();
        float visible_height = ImGui::GetWindowHeight();
        
        if (item_pos < scroll_y) {
            ImGui::SetScrollY(item_pos);
        } else if (item_pos + item_height > scroll_y + visible_height) {
            ImGui::SetScrollY(item_pos + item_height - visible_height);
        }
    }
    
    ImGui::EndChild();
    
    /* ===== Footer Section ===== */
    ImGui::Spacing();
    ImGui::Spacing();
    
    /* Decorative separator */
    ImGui::SetCursorPosX((window_width - sep_width) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.3f, 0.6f, 0.35f, 0.8f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    
    /* Instructions and quit button */
    float footer_y = ImGui::GetCursorPosY();
    
    /* Centered instructions */
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.55f, 0.5f, 1.0f));
    const char* instructions = "[ Arrow Keys ] Navigate   [ Enter/Space ] Launch   [ Esc ] Quit";
    float instr_width = ImGui::CalcTextSize(instructions).x;
    ImGui::SetCursorPosX((window_width - instr_width) * 0.5f);
    ImGui::Text("%s", instructions);
    ImGui::PopStyleColor();
    
    /* ROM count */
    ImGui::SetCursorPosY(footer_y);
    ImGui::SetCursorPosX(window_width - 150);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.45f, 0.4f, 1.0f));
    ImGui::Text("%d games", (int)count);
    ImGui::PopStyleColor();
    
    ImGui::End();
    
    return selected;
}

/* ============================================================================
 * Multi-ROM Launcher Main Loop
 * ========================================================================== */

extern "C" int chip8_run_with_menu(const RomEntry* catalog, size_t count) {
    if (!catalog || count == 0) {
        fprintf(stderr, "Error: Empty ROM catalog\n");
        return 1;
    }
    
    /* Get platform (should be SDL2 for menu support) */
    Chip8Platform* platform = chip8_get_platform();
    if (!platform) {
        fprintf(stderr, "Error: No platform registered\n");
        return 1;
    }
    
    /* Create a temporary context for menu display */
    Chip8Context menu_ctx;
    memset(&menu_ctx, 0, sizeof(menu_ctx));
    
    /* Initialize window for menu */
    if (!platform->init(&menu_ctx, "CHIP-8 Multi-ROM Launcher", 20)) {
        fprintf(stderr, "Error: Failed to initialize platform\n");
        return 1;
    }
    
    /* ImGui is already initialized by platform->init() */
    
    g_selected_rom = 0;  /* Start with first ROM selected */
    g_should_quit = false;
    
    /* ROM selection loop */
    while (!g_should_quit) {
        /* Handle SDL events */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            
            if (event.type == SDL_QUIT) {
                g_should_quit = true;
                break;
            }
            
            /* Keyboard navigation */
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        if (g_selected_rom > 0) g_selected_rom--;
                        break;
                    case SDLK_DOWN:
                        if (g_selected_rom < (int)count - 1) g_selected_rom++;
                        break;
                    case SDLK_RETURN:
                    case SDLK_SPACE:
                        /* Will be handled after rendering */
                        g_launch_selected = true;
                        break;
                    case SDLK_ESCAPE:
                        g_should_quit = true;
                        break;
                }
            }
        }
        
        if (g_should_quit) break;
        
        /* Start ImGui frame */
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        /* Render menu */
        int selected = render_rom_selection_menu(catalog, count);
        
        /* Check if keyboard launched the selection */
        if (g_launch_selected && selected < 0) {
            selected = g_selected_rom;
        }
        g_launch_selected = false;
        
        /* Finish ImGui frame */
        ImGui::Render();
        SDL_SetRenderDrawColor(g_sdl_renderer, 20, 20, 30, 255);
        SDL_RenderClear(g_sdl_renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), g_sdl_renderer);
        SDL_RenderPresent(g_sdl_renderer);
        
        /* Launch ROM if selected */
        if (selected >= 0) {
            const RomEntry* rom = &catalog[selected];
            
            /* Shutdown menu for ROM execution */
            platform->shutdown(&menu_ctx);
            
            /* Clear function table */
            chip8_clear_function_table();
            
            /* Register ROM's functions */
            if (rom->register_functions) {
                rom->register_functions();
            }
            
            /* Prepare run configuration */
            Chip8RunConfig config = CHIP8_RUN_CONFIG_DEFAULT;
            config.title = rom->title;
            config.scale = 20;
            config.cpu_freq_hz = rom->recommended_cpu_freq > 0 ? 
                                 rom->recommended_cpu_freq : 300;
            config.rom_data = rom->data;
            config.rom_size = rom->size;
            
            /* Re-initialize platform for ROM */
            chip8_set_platform(chip8_platform_sdl2());
            
            /* Enable multi-ROM mode so "Back to Menu" appears in pause menu */
            chip8_menu_set_multi_rom_mode(true);
            
            /* Run ROM */
            printf("Launching: %s\n", rom->title);
            g_return_to_menu = false;
            int result = chip8_run(rom->entry, &config);
            
            /* Check if we should return to menu or quit */
            if (g_return_to_menu) {
                /* Re-initialize for menu (platform->init also initializes ImGui) */
                if (!platform->init(&menu_ctx, "CHIP-8 Multi-ROM Launcher", 20)) {
                    fprintf(stderr, "Error: Failed to re-initialize platform\n");
                    return 1;
                }
                
                g_return_to_menu = false;
                continue;
            } else {
                /* User quit from ROM */
                return result;
            }
        }
        
        /* Frame limiting */
        SDL_Delay(16);  /* ~60 FPS */
    }
    
    /* Cleanup (platform->shutdown handles ImGui cleanup) */
    platform->shutdown(&menu_ctx);
    
    return 0;
}

/* ============================================================================
 * Menu Integration - Called from pause menu
 * ========================================================================== */

extern "C" void chip8_request_return_to_menu(void) {
    g_return_to_menu = true;
}
