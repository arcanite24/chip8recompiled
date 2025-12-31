/**
 * @file config.h
 * @brief Configuration file parsing for the recompiler
 * 
 * Supports TOML configuration files for ROM-specific settings.
 */

#ifndef RECOMPILER_CONFIG_H
#define RECOMPILER_CONFIG_H

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <set>

namespace chip8recomp {

/* ============================================================================
 * Configuration Structure
 * ========================================================================== */

/**
 * @brief Recompiler configuration
 * 
 * Loaded from TOML file or set via command-line options.
 */
struct Config {
    /* === ROM Settings === */
    
    /** Path to input ROM file */
    std::filesystem::path rom_path;
    
    /** ROM name (used for output file naming) */
    std::string rom_name = "rom";
    
    /* === Output Settings === */
    
    /** Output directory for generated files */
    std::filesystem::path output_dir = ".";
    
    /** Prefix for generated file names */
    std::string output_prefix = "rom";
    
    /** Generate all code in a single file */
    bool single_file_output = true;
    
    /* === Code Generation === */
    
    /** Include disassembly comments in output */
    bool emit_comments = true;
    
    /** Include address comments */
    bool emit_addresses = true;
    
    /** Insert timing checkpoint calls */
    bool timing_checkpoints = false;
    
    /** Embed ROM data in output (for sprites) */
    bool embed_rom = true;
    
    /* === Quirk Modes === */
    
    /** 
     * SHR/SHL use VY as source (original COSMAC VIP behavior)
     * false = Vx = Vx >> 1 (modern)
     * true  = Vx = Vy >> 1 (original)
     */
    bool quirk_shift_vy = false;
    
    /**
     * FX55/FX65 increment I register
     * true  = I = I + x + 1 after operation (original)
     * false = I unchanged (modern)
     */
    bool quirk_load_store_inc_i = true;
    
    /**
     * BNNN uses VX instead of V0
     * false = JP V0, addr (original)
     * true  = JP VX, addr (SUPER-CHIP)
     */
    bool quirk_jump_vx = false;
    
    /* === Function Overrides === */
    
    /** Manually specified function entry points */
    std::set<uint16_t> function_entry_points;
    
    /** Addresses to treat as data (not code) */
    std::set<uint16_t> data_regions_start;
    std::set<uint16_t> data_regions_end;
    
    /* === Debug === */
    
    /** Enable debug output */
    bool debug = false;
    
    /** Print disassembly during recompilation */
    bool print_disassembly = false;
    
    /** Print analysis results */
    bool print_analysis = false;
};

/* ============================================================================
 * Configuration Loading
 * ========================================================================== */

/**
 * @brief Load configuration from TOML file
 * 
 * @param path Path to TOML configuration file
 * @return Loaded configuration, or std::nullopt on error
 */
std::optional<Config> load_config(const std::filesystem::path& path);

/**
 * @brief Create default configuration for a ROM
 * 
 * @param rom_path Path to the ROM file
 * @return Default configuration
 */
Config default_config(const std::filesystem::path& rom_path);

/**
 * @brief Merge command-line options into configuration
 * 
 * Command-line options override config file settings.
 * 
 * @param config Base configuration
 * @param argc Argument count
 * @param argv Argument values
 * @return true on success, false on error
 */
bool apply_cmdline_options(Config& config, int argc, char* argv[]);

/**
 * @brief Validate configuration
 * 
 * Checks that paths exist, values are in range, etc.
 * 
 * @param config Configuration to validate
 * @param error_message Output: error message if validation fails
 * @return true if valid, false otherwise
 */
bool validate_config(const Config& config, std::string& error_message);

/**
 * @brief Print configuration summary
 * 
 * @param config Configuration to print
 */
void print_config(const Config& config);

/* ============================================================================
 * Example TOML Configuration
 * ========================================================================== */

/**
 * Example TOML configuration file:
 * 
 * ```toml
 * # pong.toml - Configuration for Pong ROM
 * 
 * [rom]
 * path = "roms/games/Pong.ch8"
 * name = "Pong"
 * 
 * [output]
 * directory = "output/pong"
 * prefix = "pong"
 * single_file = true
 * 
 * [codegen]
 * comments = true
 * addresses = true
 * embed_rom = true
 * 
 * [quirks]
 * shift_vy = false
 * load_store_inc_i = true
 * jump_vx = false
 * 
 * [functions]
 * # Override automatic function detection
 * entry_points = [0x200, 0x250]
 * 
 * [data]
 * # Mark regions as data (sprites, etc.)
 * regions = [
 *   { start = 0x2A0, end = 0x2F0 }
 * ]
 * 
 * [debug]
 * enabled = false
 * disassembly = false
 * analysis = false
 * ```
 */

} // namespace chip8recomp

#endif // RECOMPILER_CONFIG_H
