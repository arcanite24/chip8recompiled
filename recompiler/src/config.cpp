/**
 * @file config.cpp
 * @brief Configuration loading implementation
 */

#include "recompiler/config.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>

namespace chip8recomp {

std::optional<Config> load_config(const std::filesystem::path& path) {
    // TODO: Implement TOML parsing when toml11 is available
    // For now, just check if file exists and return default config
    
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: Config file not found: " << path << "\n";
        return std::nullopt;
    }
    
    std::cerr << "Warning: TOML parsing not yet implemented, using defaults\n";
    
    // Return a default config for now
    Config config;
    return config;
}

Config default_config(const std::filesystem::path& rom_path) {
    Config config;
    
    config.rom_path = rom_path;
    
    // Extract name from ROM path
    std::string name = rom_path.stem().string();
    
    // Clean up the name for use as identifier
    // Remove brackets and their contents (e.g., "[David Winter]")
    size_t bracket_start = name.find('[');
    if (bracket_start != std::string::npos) {
        name = name.substr(0, bracket_start);
    }
    
    // Remove parentheses and their contents
    size_t paren_start = name.find('(');
    if (paren_start != std::string::npos) {
        name = name.substr(0, paren_start);
    }
    
    // Trim whitespace
    name.erase(name.find_last_not_of(" \t") + 1);
    name.erase(0, name.find_first_not_of(" \t"));
    
    // Convert to lowercase and replace spaces with underscores
    std::transform(name.begin(), name.end(), name.begin(), 
                   [](unsigned char c) { 
                       return c == ' ' ? '_' : std::tolower(c); 
                   });
    
    // Remove any non-alphanumeric characters (except underscore)
    name.erase(std::remove_if(name.begin(), name.end(), 
                              [](unsigned char c) { 
                                  return !std::isalnum(c) && c != '_'; 
                              }), 
               name.end());
    
    if (name.empty()) {
        name = "rom";
    }
    
    config.rom_name = name;
    config.output_prefix = name;
    
    return config;
}

bool apply_cmdline_options(Config& config, int argc, char* argv[]) {
    // Command-line parsing is handled in main.cpp
    // This function could be expanded for more sophisticated option handling
    (void)config;
    (void)argc;
    (void)argv;
    return true;
}

bool validate_config(const Config& config, std::string& error_message) {
    // Check ROM path
    if (config.rom_path.empty()) {
        error_message = "ROM path is not specified";
        return false;
    }
    
    if (!std::filesystem::exists(config.rom_path)) {
        error_message = "ROM file does not exist: " + config.rom_path.string();
        return false;
    }
    
    // Check ROM name
    if (config.rom_name.empty()) {
        error_message = "ROM name is empty";
        return false;
    }
    
    // Check output directory is valid (if specified and exists)
    if (!config.output_dir.empty() && std::filesystem::exists(config.output_dir)) {
        if (!std::filesystem::is_directory(config.output_dir)) {
            error_message = "Output path exists but is not a directory: " + 
                           config.output_dir.string();
            return false;
        }
    }
    
    return true;
}

void print_config(const Config& config) {
    std::cout << "Configuration:\n";
    std::cout << "  ROM path: " << config.rom_path << "\n";
    std::cout << "  ROM name: " << config.rom_name << "\n";
    std::cout << "  Output dir: " << config.output_dir << "\n";
    std::cout << "  Output prefix: " << config.output_prefix << "\n";
    std::cout << "  Single file: " << (config.single_file_output ? "yes" : "no") << "\n";
    std::cout << "  Comments: " << (config.emit_comments ? "yes" : "no") << "\n";
    std::cout << "  Embed ROM: " << (config.embed_rom ? "yes" : "no") << "\n";
    std::cout << "  Quirks:\n";
    std::cout << "    shift_vy: " << (config.quirk_shift_vy ? "yes" : "no") << "\n";
    std::cout << "    load_store_inc_i: " << (config.quirk_load_store_inc_i ? "yes" : "no") << "\n";
    std::cout << "    jump_vx: " << (config.quirk_jump_vx ? "yes" : "no") << "\n";
}

} // namespace chip8recomp
