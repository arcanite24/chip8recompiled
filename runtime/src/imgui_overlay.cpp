/**
 * @file imgui_overlay.cpp
 * @brief ImGui overlay implementation for CHIP-8 debug info and settings
 */

#include "chip8rt/imgui_overlay.h"
#include "chip8rt/runtime.h"
#include "chip8rt/settings.h"
#include "chip8rt/menu.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>

/* ============================================================================
 * Helper Functions for SDL Integration
 * ========================================================================== */

/**
 * @brief Get SDL scancode name (more accurate than the C fallback)
 */
static const char* get_sdl_scancode_name(int scancode) {
    if (scancode < 0) return "None";
    const char* name = SDL_GetScancodeName((SDL_Scancode)scancode);
    if (name && name[0] != '\0') {
        return name;
    }
    return chip8_get_scancode_name(scancode);
}

/* ============================================================================
 * Initialization / Shutdown
 * ========================================================================== */

bool chip8_overlay_init(SDL_Window* window, SDL_Renderer* renderer) {
    /* Setup Dear ImGui context */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    /* Configure ImGui */
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; /* Disable imgui.ini */
    
    /* Setup style */
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.Alpha = 0.95f;
    
    /* Make it more retro-looking */
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.08f, 0.94f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.30f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.45f, 0.15f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.40f, 0.15f, 0.80f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.50f, 0.20f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.55f, 0.25f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.45f, 0.20f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.55f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.60f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.80f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.80f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.60f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.70f, 0.40f, 1.00f);
    
    /* Setup Platform/Renderer backends */
    if (!ImGui_ImplSDL2_InitForSDLRenderer(window, renderer)) {
        fprintf(stderr, "Failed to initialize ImGui SDL2 backend\n");
        return false;
    }
    
    if (!ImGui_ImplSDLRenderer2_Init(renderer)) {
        fprintf(stderr, "Failed to initialize ImGui SDL Renderer backend\n");
        ImGui_ImplSDL2_Shutdown();
        return false;
    }
    
    return true;
}

void chip8_overlay_shutdown(void) {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

/* ============================================================================
 * Event Processing
 * ========================================================================== */

bool chip8_overlay_process_event(SDL_Event* event) {
    return ImGui_ImplSDL2_ProcessEvent(event);
}

bool chip8_overlay_wants_keyboard(void) {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool chip8_overlay_wants_mouse(void) {
    return ImGui::GetIO().WantCaptureMouse;
}

/* ============================================================================
 * Frame Management
 * ========================================================================== */

void chip8_overlay_new_frame(void) {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void chip8_overlay_present(SDL_Renderer* renderer) {
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
}

/* ============================================================================
 * FPS Tracking
 * ========================================================================== */

void chip8_overlay_update_fps(Chip8OverlayState* state, uint64_t current_time_us) {
    state->frame_count++;
    
    uint64_t elapsed = current_time_us - state->last_fps_time;
    if (elapsed >= 1000000) { /* Update every second */
        state->fps = (float)state->frame_count / ((float)elapsed / 1000000.0f);
        state->frame_time_ms = 1000.0f / state->fps;
        state->frame_count = 0;
        state->last_fps_time = current_time_us;
        
        /* Store in history */
        state->fps_history[state->fps_history_idx] = state->fps;
        state->fps_history_idx = (state->fps_history_idx + 1) % 120;
    }
}

/* ============================================================================
 * Overlay Windows
 * ========================================================================== */

static void render_fps_overlay(Chip8OverlayState* state) {
    ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;
    
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.75f);
    
    if (ImGui::Begin("FPS", nullptr, flags)) {
        ImGui::Text("FPS: %.1f", state->fps);
        ImGui::Text("Frame: %.2f ms", state->frame_time_ms);
        
        /* Mini FPS graph */
        ImGui::PlotLines("##fps", state->fps_history, 120, state->fps_history_idx,
                        nullptr, 0.0f, 120.0f, ImVec2(100, 30));
    }
    ImGui::End();
}

static void render_debug_registers(Chip8Context* ctx) {
    if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen)) {
        /* General purpose registers V0-VF in 4 columns */
        ImGui::Columns(4, "regs", false);
        for (int i = 0; i < 16; i++) {
            ImGui::Text("V%X: %02X", i, ctx->V[i]);
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
        
        ImGui::Separator();
        
        /* Special registers */
        ImGui::Text("I:  %04X", ctx->I);
        ImGui::SameLine(120);
        ImGui::Text("PC: %04X", ctx->PC);
        
        ImGui::Text("DT: %02X", ctx->delay_timer);
        ImGui::SameLine(120);
        ImGui::Text("ST: %02X", ctx->sound_timer);
        
        ImGui::Text("SP: %02X", ctx->SP);
    }
}

static void render_debug_stack(Chip8Context* ctx) {
    if (ImGui::CollapsingHeader("Stack")) {
        for (int i = 0; i < ctx->SP && i < 16; i++) {
            ImGui::Text("[%02d]: %04X", i, ctx->stack[i]);
        }
        if (ctx->SP == 0) {
            ImGui::TextDisabled("(empty)");
        }
    }
}

static void render_debug_memory(Chip8Context* ctx) {
    if (ImGui::CollapsingHeader("Memory Viewer")) {
        static int mem_addr = 0x200;
        ImGui::InputInt("Address", &mem_addr, 16, 256);
        mem_addr = (mem_addr < 0) ? 0 : (mem_addr > 0xFFF) ? 0xFFF : mem_addr;
        
        /* Show 16 rows of 16 bytes */
        ImGui::BeginChild("MemView", ImVec2(0, 200), true);
        for (int row = 0; row < 16; row++) {
            int addr = (mem_addr & ~0xF) + row * 16;
            if (addr > 0xFFF) break;
            
            ImGui::Text("%04X:", addr);
            ImGui::SameLine();
            
            for (int col = 0; col < 16; col++) {
                int a = addr + col;
                if (a <= 0xFFF) {
                    if (a == ctx->PC || a == ctx->PC + 1) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%02X", ctx->memory[a]);
                    } else {
                        ImGui::Text("%02X", ctx->memory[a]);
                    }
                }
                if (col < 15) ImGui::SameLine();
            }
        }
        ImGui::EndChild();
    }
}

static void render_debug_disassembly(Chip8Context* ctx) {
    if (ImGui::CollapsingHeader("Disassembly", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("Disasm", ImVec2(0, 150), true);
        
        /* Show instructions around PC */
        int start_pc = ctx->PC - 10;
        if (start_pc < 0x200) start_pc = 0x200;
        
        for (int addr = start_pc; addr < ctx->PC + 20 && addr < 0xFFF; addr += 2) {
            uint16_t opcode = (ctx->memory[addr] << 8) | ctx->memory[addr + 1];
            
            /* Simple disassembly */
            char disasm[64];
            uint8_t x = (opcode >> 8) & 0x0F;
            uint8_t y = (opcode >> 4) & 0x0F;
            uint8_t n = opcode & 0x0F;
            uint8_t nn = opcode & 0xFF;
            uint16_t nnn = opcode & 0x0FFF;
            
            switch (opcode >> 12) {
                case 0x0:
                    if (opcode == 0x00E0) snprintf(disasm, sizeof(disasm), "CLS");
                    else if (opcode == 0x00EE) snprintf(disasm, sizeof(disasm), "RET");
                    else snprintf(disasm, sizeof(disasm), "SYS %03X", nnn);
                    break;
                case 0x1: snprintf(disasm, sizeof(disasm), "JP %03X", nnn); break;
                case 0x2: snprintf(disasm, sizeof(disasm), "CALL %03X", nnn); break;
                case 0x3: snprintf(disasm, sizeof(disasm), "SE V%X, %02X", x, nn); break;
                case 0x4: snprintf(disasm, sizeof(disasm), "SNE V%X, %02X", x, nn); break;
                case 0x5: snprintf(disasm, sizeof(disasm), "SE V%X, V%X", x, y); break;
                case 0x6: snprintf(disasm, sizeof(disasm), "LD V%X, %02X", x, nn); break;
                case 0x7: snprintf(disasm, sizeof(disasm), "ADD V%X, %02X", x, nn); break;
                case 0x8:
                    switch (n) {
                        case 0x0: snprintf(disasm, sizeof(disasm), "LD V%X, V%X", x, y); break;
                        case 0x1: snprintf(disasm, sizeof(disasm), "OR V%X, V%X", x, y); break;
                        case 0x2: snprintf(disasm, sizeof(disasm), "AND V%X, V%X", x, y); break;
                        case 0x3: snprintf(disasm, sizeof(disasm), "XOR V%X, V%X", x, y); break;
                        case 0x4: snprintf(disasm, sizeof(disasm), "ADD V%X, V%X", x, y); break;
                        case 0x5: snprintf(disasm, sizeof(disasm), "SUB V%X, V%X", x, y); break;
                        case 0x6: snprintf(disasm, sizeof(disasm), "SHR V%X", x); break;
                        case 0x7: snprintf(disasm, sizeof(disasm), "SUBN V%X, V%X", x, y); break;
                        case 0xE: snprintf(disasm, sizeof(disasm), "SHL V%X", x); break;
                        default: snprintf(disasm, sizeof(disasm), "??? %04X", opcode); break;
                    }
                    break;
                case 0x9: snprintf(disasm, sizeof(disasm), "SNE V%X, V%X", x, y); break;
                case 0xA: snprintf(disasm, sizeof(disasm), "LD I, %03X", nnn); break;
                case 0xB: snprintf(disasm, sizeof(disasm), "JP V0, %03X", nnn); break;
                case 0xC: snprintf(disasm, sizeof(disasm), "RND V%X, %02X", x, nn); break;
                case 0xD: snprintf(disasm, sizeof(disasm), "DRW V%X, V%X, %X", x, y, n); break;
                case 0xE:
                    if (nn == 0x9E) snprintf(disasm, sizeof(disasm), "SKP V%X", x);
                    else if (nn == 0xA1) snprintf(disasm, sizeof(disasm), "SKNP V%X", x);
                    else snprintf(disasm, sizeof(disasm), "??? %04X", opcode);
                    break;
                case 0xF:
                    switch (nn) {
                        case 0x07: snprintf(disasm, sizeof(disasm), "LD V%X, DT", x); break;
                        case 0x0A: snprintf(disasm, sizeof(disasm), "LD V%X, K", x); break;
                        case 0x15: snprintf(disasm, sizeof(disasm), "LD DT, V%X", x); break;
                        case 0x18: snprintf(disasm, sizeof(disasm), "LD ST, V%X", x); break;
                        case 0x1E: snprintf(disasm, sizeof(disasm), "ADD I, V%X", x); break;
                        case 0x29: snprintf(disasm, sizeof(disasm), "LD F, V%X", x); break;
                        case 0x33: snprintf(disasm, sizeof(disasm), "LD B, V%X", x); break;
                        case 0x55: snprintf(disasm, sizeof(disasm), "LD [I], V%X", x); break;
                        case 0x65: snprintf(disasm, sizeof(disasm), "LD V%X, [I]", x); break;
                        default: snprintf(disasm, sizeof(disasm), "??? %04X", opcode); break;
                    }
                    break;
                default:
                    snprintf(disasm, sizeof(disasm), "??? %04X", opcode);
                    break;
            }
            
            if (addr == ctx->PC) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "> %04X: %04X  %s", addr, opcode, disasm);
            } else {
                ImGui::Text("  %04X: %04X  %s", addr, opcode, disasm);
            }
        }
        ImGui::EndChild();
    }
}

static void render_debug_keys(Chip8Context* ctx) {
    if (ImGui::CollapsingHeader("Keypad State")) {
        ImGui::Columns(4, "keys", false);
        const char* key_labels[] = {"1", "2", "3", "C", "4", "5", "6", "D", "7", "8", "9", "E", "A", "0", "B", "F"};
        const int key_order[] = {1, 2, 3, 0xC, 4, 5, 6, 0xD, 7, 8, 9, 0xE, 0xA, 0, 0xB, 0xF};
        for (int i = 0; i < 16; i++) {
            int k = key_order[i];
            if (ctx->keys[k]) {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[%s]", key_labels[i]);
            } else {
                ImGui::TextDisabled(" %s ", key_labels[i]);
            }
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }
}

static void render_debug_overlay(Chip8Context* ctx, Chip8OverlayState* state) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;
    
    ImGui::SetNextWindowPos(ImVec2(10, 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 500), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("CHIP-8 Debug", &state->show_debug, flags)) {
        render_debug_registers(ctx);
        render_debug_stack(ctx);
        render_debug_keys(ctx);
        render_debug_disassembly(ctx);
        render_debug_memory(ctx);
    }
    ImGui::End();
}

static void render_settings_window(Chip8Settings* settings, Chip8OverlayState* state) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;
    
    ImGui::SetNextWindowPos(ImVec2(400, 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Settings", &state->show_settings, flags)) {
        bool changed = false;  /* Track if any setting was modified */
        
        if (ImGui::CollapsingHeader("Graphics", ImGuiTreeNodeFlags_DefaultOpen)) {
            /* Window Size */
            const char* size_names[] = {"1x (64x32)", "2x (128x64)", "5x (320x160)", 
                                        "10x (640x320)", "15x (960x480)", "20x (1280x640)", "Custom"};
            int current_size = (int)settings->graphics.window_size;
            if (ImGui::Combo("Window Size", &current_size, size_names, 7)) {
                settings->graphics.window_size = (Chip8WindowSize)current_size;
                /* Also update scale based on preset */
                if (settings->graphics.window_size != CHIP8_WINDOW_CUSTOM) {
                    settings->graphics.scale = chip8_get_window_size_scale(settings->graphics.window_size);
                }
                changed = true;
            }
            
            /* Custom scale (only if Custom selected) */
            if (settings->graphics.window_size == CHIP8_WINDOW_CUSTOM) {
                int scale = settings->graphics.scale;
                if (ImGui::SliderInt("Custom Scale", &scale, 1, 30)) {
                    settings->graphics.scale = scale;
                    changed = true;
                }
            }
            
            ImGui::Separator();
            
            /* Visual Effects */
            bool pixel_grid = settings->graphics.pixel_grid;
            if (ImGui::Checkbox("Pixel Grid", &pixel_grid)) {
                settings->graphics.pixel_grid = pixel_grid;
                changed = true;
            }
            
            bool crt = settings->graphics.crt_effect;
            if (ImGui::Checkbox("CRT Effect", &crt)) {
                settings->graphics.crt_effect = crt;
                changed = true;
            }
            
            if (settings->graphics.crt_effect) {
                float intensity = settings->graphics.scanline_intensity * 100.0f;
                if (ImGui::SliderFloat("Scanline Intensity", &intensity, 0.0f, 100.0f, "%.0f%%")) {
                    settings->graphics.scanline_intensity = intensity / 100.0f;
                    changed = true;
                }
            }
            
            ImGui::Separator();
            
            /* Colors */
            float fg[3] = {
                settings->graphics.custom_fg.r / 255.0f,
                settings->graphics.custom_fg.g / 255.0f,
                settings->graphics.custom_fg.b / 255.0f
            };
            if (ImGui::ColorEdit3("Foreground", fg)) {
                settings->graphics.custom_fg.r = (uint8_t)(fg[0] * 255);
                settings->graphics.custom_fg.g = (uint8_t)(fg[1] * 255);
                settings->graphics.custom_fg.b = (uint8_t)(fg[2] * 255);
                settings->graphics.theme = CHIP8_THEME_CUSTOM;
                changed = true;
            }
            
            float bg[3] = {
                settings->graphics.custom_bg.r / 255.0f,
                settings->graphics.custom_bg.g / 255.0f,
                settings->graphics.custom_bg.b / 255.0f
            };
            if (ImGui::ColorEdit3("Background", bg)) {
                settings->graphics.custom_bg.r = (uint8_t)(bg[0] * 255);
                settings->graphics.custom_bg.g = (uint8_t)(bg[1] * 255);
                settings->graphics.custom_bg.b = (uint8_t)(bg[2] * 255);
                settings->graphics.theme = CHIP8_THEME_CUSTOM;
                changed = true;
            }
        }
        
        if (ImGui::CollapsingHeader("Audio")) {
            bool muted = settings->audio.muted;
            if (ImGui::Checkbox("Mute Audio", &muted)) {
                settings->audio.muted = muted;
                changed = true;
            }
            
            float volume = settings->audio.volume * 100.0f;
            if (ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.0f%%")) {
                settings->audio.volume = volume / 100.0f;
                changed = true;
            }
            
            int frequency = settings->audio.frequency;
            if (ImGui::SliderInt("Tone Frequency", &frequency, 200, 1000)) {
                settings->audio.frequency = frequency;
                changed = true;
            }
        }
        
        if (ImGui::CollapsingHeader("Gameplay")) {
            int speed = settings->gameplay.cpu_freq_hz;
            if (ImGui::SliderInt("CPU Speed (Hz)", &speed, 100, 2000)) {
                settings->gameplay.cpu_freq_hz = speed;
                changed = true;
            }
            
            bool quirks = settings->gameplay.quirks.shift_uses_vy;
            if (ImGui::Checkbox("Shift uses VY (COSMAC)", &quirks)) {
                settings->gameplay.quirks.shift_uses_vy = quirks;
                changed = true;
            }
            
            quirks = settings->gameplay.quirks.memory_increment_i;
            if (ImGui::Checkbox("Memory ops increment I", &quirks)) {
                settings->gameplay.quirks.memory_increment_i = quirks;
                changed = true;
            }
        }
        
        if (ImGui::CollapsingHeader("Controls")) {
            ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Keyboard Mappings");
            ImGui::Separator();
            
            /* CHIP-8 keypad layout visual */
            ImGui::Text("CHIP-8 Keypad Layout:");
            ImGui::TextDisabled(" 1 2 3 C");
            ImGui::TextDisabled(" 4 5 6 D");
            ImGui::TextDisabled(" 7 8 9 E");
            ImGui::TextDisabled(" A 0 B F");
            ImGui::Spacing();
            
            /* Key bindings table */
            if (ImGui::BeginTable("KeyBindings", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Primary", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Alternate", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Gamepad", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                
                for (int i = 0; i < 16; i++) {
                    ImGui::TableNextRow();
                    
                    /* Key label */
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", chip8_get_key_label(i));
                    
                    /* Primary keyboard binding */
                    ImGui::TableSetColumnIndex(1);
                    char btn_label[64];
                    const char* key_name = get_sdl_scancode_name(settings->input.bindings[i].keyboard);
                    snprintf(btn_label, sizeof(btn_label), "%s##kb%d", key_name, i);
                    if (ImGui::Button(btn_label, ImVec2(-1, 0))) {
                        state->remap_target_key = i;
                        state->remap_is_gamepad = false;
                        state->remap_is_alternate = false;
                        state->waiting_for_input = true;
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Click and press a key to remap");
                    }
                    
                    /* Alternate keyboard binding */
                    ImGui::TableSetColumnIndex(2);
                    const char* alt_name = get_sdl_scancode_name(settings->input.bindings[i].keyboard_alt);
                    snprintf(btn_label, sizeof(btn_label), "%s##kbalt%d", alt_name, i);
                    if (ImGui::Button(btn_label, ImVec2(-1, 0))) {
                        state->remap_target_key = i;
                        state->remap_is_gamepad = false;
                        state->remap_is_alternate = true;
                        state->waiting_for_input = true;
                    }
                    
                    /* Gamepad binding */
                    ImGui::TableSetColumnIndex(3);
                    const char* gpad_name = chip8_get_gamepad_button_name(settings->input.bindings[i].gamepad_button);
                    snprintf(btn_label, sizeof(btn_label), "%s##gp%d", gpad_name, i);
                    if (ImGui::Button(btn_label, ImVec2(-1, 0))) {
                        state->remap_target_key = i;
                        state->remap_is_gamepad = true;
                        state->remap_is_alternate = false;
                        state->waiting_for_input = true;
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Click and press a gamepad button");
                    }
                }
                ImGui::EndTable();
            }
            
            /* Waiting for input indicator */
            if (state->waiting_for_input) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                    "Press a %s for key %s (ESC to cancel)...",
                    state->remap_is_gamepad ? "gamepad button" : "keyboard key",
                    chip8_get_key_label(state->remap_target_key));
            }
            
            ImGui::Spacing();
            if (ImGui::Button("Reset to Defaults##keys")) {
                chip8_input_settings_default(&settings->input);
                changed = true;
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Gamepad Settings");
            ImGui::Separator();
            
            /* Show connected controllers info */
            int num_joysticks = SDL_NumJoysticks();
            int num_controllers = 0;
            for (int i = 0; i < num_joysticks; i++) {
                if (SDL_IsGameController(i)) num_controllers++;
            }
            
            if (num_controllers > 0) {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%d Controller(s) Detected", num_controllers);
                for (int i = 0; i < num_joysticks && i < CHIP8_MAX_GAMEPADS; i++) {
                    if (SDL_IsGameController(i)) {
                        SDL_GameController* gc = SDL_GameControllerOpen(i);
                        if (gc) {
                            const char* name = SDL_GameControllerName(gc);
                            bool has_rumble = SDL_GameControllerHasRumble(gc);
                            ImGui::BulletText("%s %s", name ? name : "Unknown", 
                                            has_rumble ? "(Rumble)" : "");
                            SDL_GameControllerClose(gc);
                        }
                    }
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "No Controllers Detected");
                ImGui::TextDisabled("Connect a controller and it will be detected automatically");
            }
            
            ImGui::Spacing();
            
            bool gamepad_enabled = settings->input.gamepad_enabled;
            if (ImGui::Checkbox("Enable Gamepad", &gamepad_enabled)) {
                settings->input.gamepad_enabled = gamepad_enabled;
                changed = true;
            }
            
            if (settings->input.gamepad_enabled) {
                /* Active gamepad selector */
                char active_label[128];
                snprintf(active_label, sizeof(active_label), "Controller %d", 
                        settings->input.active_gamepad + 1);
                if (ImGui::BeginCombo("Active Controller", active_label)) {
                    for (int i = 0; i < CHIP8_MAX_GAMEPADS; i++) {
                        char label[64];
                        snprintf(label, sizeof(label), "Controller %d", i + 1);
                        bool selected = (settings->input.active_gamepad == i);
                        if (ImGui::Selectable(label, selected)) {
                            settings->input.active_gamepad = i;
                            changed = true;
                        }
                    }
                    ImGui::EndCombo();
                }
                
                float deadzone = settings->input.analog_deadzone * 100.0f;
                if (ImGui::SliderFloat("Analog Deadzone", &deadzone, 0.0f, 50.0f, "%.0f%%")) {
                    settings->input.analog_deadzone = deadzone / 100.0f;
                    changed = true;
                }
                
                bool use_stick = settings->input.use_left_stick;
                if (ImGui::Checkbox("Use Left Stick for Direction", &use_stick)) {
                    settings->input.use_left_stick = use_stick;
                    changed = true;
                }
                
                bool use_dpad = settings->input.use_dpad;
                if (ImGui::Checkbox("Use D-Pad for Direction", &use_dpad)) {
                    settings->input.use_dpad = use_dpad;
                    changed = true;
                }
                
                ImGui::Separator();
                
                bool vibration = settings->input.vibration_enabled;
                if (ImGui::Checkbox("Vibration Feedback", &vibration)) {
                    settings->input.vibration_enabled = vibration;
                    changed = true;
                }
                
                if (settings->input.vibration_enabled) {
                    float vib_intensity = settings->input.vibration_intensity * 100.0f;
                    if (ImGui::SliderFloat("Vibration Intensity", &vib_intensity, 0.0f, 100.0f, "%.0f%%")) {
                        settings->input.vibration_intensity = vib_intensity / 100.0f;
                        changed = true;
                    }
                    
                    if (ImGui::Button("Test Vibration")) {
                        /* This will be handled by the platform layer */
                        state->settings_changed = true;
                    }
                }
            }
        }
        
        if (ImGui::CollapsingHeader("Overlay")) {
            ImGui::Checkbox("Show FPS", &state->show_fps);
            ImGui::Checkbox("Show Debug Window", &state->show_debug);
            ImGui::Checkbox("Show ImGui Demo", &state->show_demo);
        }
        
        /* Game Control Buttons */
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        /* Resume button */
        if (ImGui::Button("Resume", ImVec2(-1, 30))) {
            state->show_settings = false;
        }
        
        /* Reset button */
        if (ImGui::Button("Reset Game", ImVec2(-1, 30))) {
            state->reset_requested = true;
            state->show_settings = false;
        }
        
        /* Back to Menu button (only in multi-ROM mode) */
        if (chip8_menu_is_multi_rom_mode()) {
            ImGui::Spacing();
            if (ImGui::Button("Back to Menu", ImVec2(-1, 30))) {
                state->back_to_menu_requested = true;
                state->show_settings = false;
            }
        }
        
        ImGui::Spacing();
        
        /* Quit button */
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Quit", ImVec2(-1, 30))) {
            state->quit_requested = true;
        }
        ImGui::PopStyleColor(3);
        
        /* Mark settings as changed so platform can apply them */
        if (changed) {
            state->settings_changed = true;
        }
    }
    ImGui::End();
}

static void render_rom_info(Chip8Context* ctx, Chip8OverlayState* state) {
    ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;
    
    /* Position at top-right */
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10, 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.75f);
    
    if (ImGui::Begin("ROM Info", &state->show_rom_info, flags)) {
        ImGui::Text("PC: %04X", ctx->PC);
        ImGui::Text("Instructions: %llu", (unsigned long long)ctx->instruction_count);
    }
    ImGui::End();
}

/* ============================================================================
 * Main Render Function
 * ========================================================================== */

void chip8_overlay_render(Chip8Context* ctx, Chip8OverlayState* state, Chip8Settings* settings) {
    if (state->show_fps) {
        render_fps_overlay(state);
    }
    
    if (state->show_debug && ctx) {
        render_debug_overlay(ctx, state);
    }
    
    if (state->show_settings && settings) {
        render_settings_window(settings, state);
    }
    
    if (state->show_rom_info && ctx) {
        render_rom_info(ctx, state);
    }
    
    if (state->show_demo) {
        ImGui::ShowDemoWindow(&state->show_demo);
    }
}

/* ============================================================================
 * Toggle Functions
 * ========================================================================== */

void chip8_overlay_toggle_debug(Chip8OverlayState* state) {
    state->show_debug = !state->show_debug;
}

void chip8_overlay_toggle_fps(Chip8OverlayState* state) {
    state->show_fps = !state->show_fps;
}
